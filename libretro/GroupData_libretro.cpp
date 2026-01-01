/*
 * SpaceCadetPinball libretro core
 * GroupData replacement with proper font loading
 */

#include "libretro_pch.h"
#include "../SpaceCadetPinball/GroupData.h"
#include "../SpaceCadetPinball/fullscrn.h"
#include "../SpaceCadetPinball/gdrv.h"
#include "../SpaceCadetPinball/pb.h"
#include "../SpaceCadetPinball/zdrv.h"
#include "../SpaceCadetPinball/EmbeddedData.h"

// Base85 decoding for embedded font data (ImGui compatible format)
static unsigned int Decode85Byte(char c) {
    return (c >= '\\') ? (unsigned int)(c - 36) : (unsigned int)(c - 35);
}

static void* DecompressBase85(const char* src, unsigned int* out_size) {
    // First pass: calculate output size
    unsigned int src_size = (unsigned int)strlen(src);
    unsigned int dst_size = ((src_size + 4) / 5) * 4;
    
    // Allocate output buffer
    unsigned char* dst = new unsigned char[dst_size];
    unsigned char* dst_ptr = dst;
    
    while (*src) {
        unsigned int d = 0;
        for (int n = 0; n < 5 && *src; n++, src++) {
            d = d * 85 + Decode85Byte(*src);
        }
        for (int n = 3; n >= 0; n--) {
            dst_ptr[n] = (unsigned char)(d & 0xFF);
            d >>= 8;
        }
        dst_ptr += 4;
    }
    
    // Now decompress STB-style compressed data
    // The base85 data contains STB compressed data
    unsigned char* compressed = dst;
    unsigned int compressed_size = (unsigned int)(dst_ptr - dst);
    
    // Read uncompressed size from first 4 bytes (little endian)
    unsigned int uncompressed_size = compressed[0] | (compressed[1] << 8) | 
                                     (compressed[2] << 16) | (compressed[3] << 24);
    
    // Allocate final output
    unsigned char* output = new unsigned char[uncompressed_size];
    
    // Simple RLE decompression (STB format)
    unsigned char* src_ptr = compressed + 4;
    unsigned char* src_end = compressed + compressed_size;
    unsigned char* out_ptr = output;
    unsigned char* out_end = output + uncompressed_size;
    
    while (src_ptr < src_end && out_ptr < out_end) {
        unsigned char token = *src_ptr++;
        if (token < 0x80) {
            // Literal run: copy token+1 bytes
            unsigned int len = token + 1;
            while (len-- && src_ptr < src_end && out_ptr < out_end) {
                *out_ptr++ = *src_ptr++;
            }
        } else {
            // Repeat run: repeat next byte (token - 0x7F) times
            unsigned int len = token - 0x7F;
            unsigned char val = (src_ptr < src_end) ? *src_ptr++ : 0;
            while (len-- && out_ptr < out_end) {
                *out_ptr++ = val;
            }
        }
    }
    
    delete[] dst;
    
    if (out_size) *out_size = uncompressed_size;
    return output;
}

// GroupData implementation
GroupData::GroupData(int groupId) : GroupId(groupId), GroupName(), Bitmaps{}, ZMaps{}, NeedsSort(false)
{
}

void GroupData::AddEntry(EntryData* entry)
{
    bool addEntry = true;
    
    switch (entry->EntryType)
    {
    case FieldTypes::GroupName:
        GroupName = entry->Buffer;
        break;
    case FieldTypes::Bitmap8bit:
        {
            auto srcBmp = reinterpret_cast<gdrv_bitmap8*>(entry->Buffer);
            if (srcBmp->BitmapType == BitmapTypes::Spliced)
            {
                // Get rid of spliced bitmap early on, to simplify render pipeline
                auto bmp = new gdrv_bitmap8(srcBmp->Width, srcBmp->Height, true);
                auto zMap = new zmap_header_type(srcBmp->Width, srcBmp->Height, srcBmp->Width);
                SplitSplicedBitmap(*srcBmp, *bmp, *zMap);

                NeedsSort = true;
                addEntry = false;
                AddEntry(new EntryData(FieldTypes::Bitmap8bit, reinterpret_cast<char*>(bmp)));
                AddEntry(new EntryData(FieldTypes::Bitmap16bit, reinterpret_cast<char*>(zMap)));
                delete entry;
            }
            else
            {
                SetBitmap(srcBmp);
            }
            break;
        }
    case FieldTypes::Bitmap16bit:
        {
            SetZMap(reinterpret_cast<zmap_header_type*>(entry->Buffer));
            break;
        }
    default:
        break;
    }

    if (addEntry)
        Entries.push_back(entry);
}

void GroupData::SetBitmap(gdrv_bitmap8* bmp)
{
    if (bmp->Resolution < 3)
        Bitmaps[bmp->Resolution] = bmp;
}

void GroupData::SetZMap(zmap_header_type* zMap)
{
    // Flip zMap to match with flipped non-indexed bitmaps
    zdrv::FlipZMapHorizontally(*zMap);
    
    if (zMap->Resolution < 3)
        ZMaps[zMap->Resolution] = zMap;
}

void GroupData::SplitSplicedBitmap(const gdrv_bitmap8& srcBmp, gdrv_bitmap8& bmp, zmap_header_type& zMap)
{
    std::memset(bmp.IndexedBmpPtr, 0xff, bmp.Stride * bmp.Height);
    bmp.XPosition = srcBmp.XPosition;
    bmp.YPosition = srcBmp.YPosition;
    bmp.Resolution = srcBmp.Resolution;

    zdrv::fill(&zMap, zMap.Width, zMap.Height, 0, 0, 0xFFFF);
    zMap.Resolution = srcBmp.Resolution;

    auto tableWidth = fullscrn::resolution_array[srcBmp.Resolution].TableWidth;
    auto src = reinterpret_cast<uint16_t*>(srcBmp.IndexedBmpPtr);
    auto srcChar = reinterpret_cast<char**>(&src);
    for (int dstInd = 0;;)
    {
        auto stride = static_cast<int16_t>(*src++);
        if (stride < 0)
            break;

        // Stride is in terms of dst stride, hardcoded to match vScreen width in current resolution
        if (stride > bmp.Width)
        {
            stride += bmp.Width - tableWidth;
        }

        dstInd += stride;
        for (auto count = *src++; count; count--)
        {
            auto depth = *src++;
            bmp.IndexedBmpPtr[dstInd] = **srcChar;
            zMap.ZPtr1[dstInd] = depth;

            (*srcChar)++;
            dstInd++;
        }
    }
}

void GroupData::FinalizeGroup()
{
    if (NeedsSort)
    {
        // Sort bitmaps by resolution
        for (int i = 0; i < 3; i++)
        {
            if (!Bitmaps[i])
            {
                for (int j = i + 1; j < 3; j++)
                {
                    if (Bitmaps[j])
                    {
                        Bitmaps[i] = Bitmaps[j];
                        Bitmaps[j] = nullptr;
                        break;
                    }
                }
            }
        }
    }
}

gdrv_bitmap8* GroupData::GetBitmap(int resolution) const
{
    if (resolution < 0 || resolution >= 3)
        return Bitmaps[0];
    return Bitmaps[resolution] ? Bitmaps[resolution] : Bitmaps[0];
}

zmap_header_type* GroupData::GetZMap(int resolution) const
{
    if (resolution < 0 || resolution >= 3)
        return ZMaps[0];
    return ZMaps[resolution] ? ZMaps[resolution] : ZMaps[0];
}

// EntryData implementation - only destructor needed (constructors are inline in header)
EntryData::~EntryData()
{
    if (Buffer)
    {
        if (EntryType == FieldTypes::Bitmap8bit)
            delete reinterpret_cast<gdrv_bitmap8*>(Buffer);
        else if (EntryType == FieldTypes::Bitmap16bit)
            delete reinterpret_cast<zmap_header_type*>(Buffer);
        else
            delete[] Buffer;
    }
}

// DatFile implementation
DatFile::~DatFile()
{
    for (auto group : Groups)
        delete group;
}

char* DatFile::field_nth(int groupIndex, FieldTypes targetEntryType, int skipFirstN)
{
    if (groupIndex < 0 || groupIndex >= static_cast<int>(Groups.size()))
        return nullptr;
    auto group = Groups[groupIndex];
    for (auto entry : group->GetEntries())
    {
        if (entry->EntryType == targetEntryType)
        {
            if (skipFirstN <= 0)
                return entry->Buffer;
            skipFirstN--;
        }
    }
    return nullptr;
}

char* DatFile::field(int groupIndex, FieldTypes entryType)
{
    return field_nth(groupIndex, entryType, 0);
}

int DatFile::field_size_nth(int groupIndex, FieldTypes targetEntryType, int skipFirstN)
{
    if (groupIndex < 0 || groupIndex >= static_cast<int>(Groups.size()))
        return 0;
    auto group = Groups[groupIndex];
    for (auto entry : group->GetEntries())
    {
        if (entry->EntryType == targetEntryType)
        {
            if (skipFirstN <= 0)
                return entry->FieldSize;
            skipFirstN--;
        }
    }
    return 0;
}

int DatFile::field_size(int groupIndex, FieldTypes targetEntryType)
{
    return field_size_nth(groupIndex, targetEntryType, 0);
}

int DatFile::record_labeled(LPCSTR targetGroupName)
{
    for (size_t i = 0; i < Groups.size(); i++)
    {
        if (Groups[i]->GroupName == targetGroupName)
            return static_cast<int>(i);
    }
    return -1;
}

char* DatFile::field_labeled(LPCSTR lpString, FieldTypes fieldType)
{
    int index = record_labeled(lpString);
    if (index < 0)
        return nullptr;
    return field(index, fieldType);
}

gdrv_bitmap8* DatFile::GetBitmap(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= static_cast<int>(Groups.size()))
        return nullptr;
    return Groups[groupIndex]->GetBitmap(fullscrn::GetResolution());
}

zmap_header_type* DatFile::GetZMap(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= static_cast<int>(Groups.size()))
        return nullptr;
    return Groups[groupIndex]->GetZMap(fullscrn::GetResolution());
}

void DatFile::Finalize()
{
    // Load embedded font for 3DPB mode (not Full Tilt)
    if (!pb::FullTiltMode)
    {
        int groupIndex = record_labeled("pbmsg_ft");
        if (groupIndex < 0) // Font not already in dat
        {
            // Decompress the embedded font data
            unsigned int fontDataSize = 0;
            auto rcData = reinterpret_cast<MsgFont*>(DecompressBase85(
                EmbeddedData::PB_MSGFT_bin_compressed_data_base85, &fontDataSize));
            
            if (rcData && fontDataSize > 0)
            {
                AddMsgFont(rcData, "pbmsg_ft");
                delete[] reinterpret_cast<unsigned char*>(rcData);
            }
        }
    }
    
    for (auto group : Groups)
    {
        group->FinalizeGroup();
    }
}

void DatFile::AddMsgFont(MsgFont* font, const std::string& fontName)
{
    if (!font) return;
    
    auto groupId = Groups.back()->GroupId + 1;
    auto ptrToData = reinterpret_cast<char*>(font->Data);
    
    for (auto charInd = 32; charInd < 128; charInd++, groupId++)
    {
        auto curChar = reinterpret_cast<MsgFontChar*>(ptrToData);
        ptrToData += curChar->Width * font->Height + 1;
        
        auto bmp = new gdrv_bitmap8(curChar->Width, font->Height, true);
        auto srcPtr = curChar->Data;
        auto dstPtr = &bmp->IndexedBmpPtr[bmp->Stride * (bmp->Height - 1)];
        for (auto y = 0; y < font->Height; ++y)
        {
            memcpy(dstPtr, srcPtr, curChar->Width);
            srcPtr += curChar->Width;
            dstPtr -= bmp->Stride;
        }
        
        auto group = new GroupData(groupId);
        group->AddEntry(new EntryData(FieldTypes::Bitmap8bit, reinterpret_cast<char*>(bmp)));
        
        if (charInd == 32)
        {
            // First font group holds font name and gap width
            auto groupNameBuf = new char[fontName.length() + 1];
            strcpy(groupNameBuf, fontName.c_str());
            group->AddEntry(new EntryData(FieldTypes::GroupName, groupNameBuf));
            
            auto gaps = new char[2];
            *reinterpret_cast<int16_t*>(gaps) = font->GapWidth;
            group->AddEntry(new EntryData(FieldTypes::ShortArray, gaps));
        }
        else
        {
            auto groupNameBuf = new char[30];
            sprintf(groupNameBuf, "char %d='%c'", charInd, charInd);
            group->AddEntry(new EntryData(FieldTypes::GroupName, groupNameBuf));
        }
        
        Groups.push_back(group);
    }
}
