#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t Jmp[3];
    uint8_t Oem[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t RootDirEntryCount;
    uint16_t TotalSectorsShort;
    uint8_t MediaDescriptorType;
    uint16_t Unused; // Sectors per fat12/16 (unused for fat32).
    uint16_t SectorsPerTrack;
    uint16_t HeadCount;
    uint32_t HiddenSectorsCount;
    uint32_t TotalSectorsLong;

    // Extended boot record.
    uint32_t SectorsPerFat;
    uint16_t Flags;
    uint16_t FatVersionNumber;
    uint32_t RootDirCluster;
    uint16_t FsInfoSector;
    uint16_t BackupBootSector;
    uint8_t Reserved[12];
    uint8_t DriveNumber;
    uint8_t Reserved1;
    uint8_t Signature;
    uint32_t VolumeIDSerial;
    char VolumeLabel[11];
    uint8_t SystemID[8];
    uint8_t BootCode[420];
    uint16_t BootSignature; // Should be 0xAA55
} __attribute__((packed)) Fat32BPB;

typedef struct {
    uint32_t LeadSignature;
    uint8_t Reserved[480];
    uint32_t Signature;
    uint32_t LastFreeClusterCount;
    uint32_t FirstFreeCluster;
    uint8_t Reserved1[12];
    uint32_t TrailSignature;
} __attribute__((packed)) Fat32FsInfo;

typedef struct {
    uint8_t FileName[11]; // First 8 bytes are file name, last 3 are extension.
    uint8_t Attributes;
    uint8_t Reserved;
    uint8_t CreationTimeHundredths;
    uint16_t CreationTime; // 5 Bits for hour, 6 Bits for minutes, 5 Bits for seconds (multiply seconds by 2).
    uint16_t CreationDate; // 7 Bits for year, 4 Bits for month, 5 Bits for day.
    uint16_t LastAccessData; // Same layout as creation date.
    uint16_t HiFirstCluster; // Higher 16 bits of first cluster number for this entry.
    uint16_t LastModificationTime; // Same layout as creation time.
    uint16_t LastModificationDate; // Same layout as creation date.
    uint16_t LoFirstCluster; // Lower 16 bits of first cluster number for this entry.
    uint32_t FileSize;
} __attribute__((packed)) Fat32Dir;

typedef struct {
    char *pName;
    uint32_t Size;
    uint32_t Sector;
    uint32_t Cluster;
} __attribute__((packed)) Fat32Entry;

typedef struct {
    Fat32BPB *pBPB;
    Fat32FsInfo *pFsInfo;
    uint64_t FirstDataSector;

    void(*DiskReadSector)(uint8_t *pBuffer, uint64_t Sector, uint64_t Size);
} __attribute__((packed)) Fat32FS;

Fat32FS *FsFatInit(void(*DiskReadSector)(uint8_t *pBuffer, uint64_t Sector, uint64_t Size));