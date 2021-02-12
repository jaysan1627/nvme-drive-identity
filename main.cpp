#include <windows.h>
#include <stdio.h>
#include <intrin.h>

typedef enum {
    NVME_IDENTIFY_CNS_SPECIFIC_NAMESPACE = 0,
    NVME_IDENTIFY_CNS_CONTROLLER = 1,
    NVME_IDENTIFY_CNS_ACTIVE_NAMESPACES = 2
} NVME_IDENTIFY_CNS_CODES;

int main() 
{
    printf("Looping your drives now ...\n\n");

    for (int i = 0; i < 26; ++i)
    {
        wchar_t current_drive[21]{};
        wsprintf(current_drive, L"\\\\.\\PhysicalDrive%d", i);

        HANDLE drive_handle = CreateFileW(current_drive, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (!drive_handle || drive_handle == INVALID_HANDLE_VALUE)
            continue;

        size_t buffer_size = (offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + 4096);
        STORAGE_PROPERTY_QUERY* query = (STORAGE_PROPERTY_QUERY*)malloc(buffer_size);

        if (!query)
        {
            CloseHandle(drive_handle);
            printf("[!] Failed allocating buffer -> 0x%X ...\n", GetLastError());
            continue;
        }

        query->PropertyId = StorageAdapterProtocolSpecificProperty;
        query->QueryType = PropertyStandardQuery;

        STORAGE_PROTOCOL_SPECIFIC_DATA* specific_data = (STORAGE_PROTOCOL_SPECIFIC_DATA*)query->AdditionalParameters;
        specific_data->ProtocolType = ProtocolTypeNvme;
        specific_data->DataType = NVMeDataTypeIdentify;
        specific_data->ProtocolDataRequestValue = NVME_IDENTIFY_CNS_CONTROLLER;
        specific_data->ProtocolDataRequestSubValue = 0;
        specific_data->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
        specific_data->ProtocolDataLength = 4096;

        DWORD read_bytes = 0;
        if (!DeviceIoControl(drive_handle, IOCTL_STORAGE_QUERY_PROPERTY, query, buffer_size, query, buffer_size, &read_bytes, nullptr))
        {
            free(query);
            CloseHandle(drive_handle);

            wprintf(L"[!] Invalid drive -> %s\n[!] Failed IOCTL -> 0x%X ...\n", current_drive, GetLastError());
            continue;
        }

        wprintf(L"\n[+] Valid drive -> %s\n", current_drive);

        CloseHandle(drive_handle);

        char* data = ((char*)query + FIELD_OFFSET(STORAGE_PROPERTY_QUERY, AdditionalParameters) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + 0x4);

        char serial_number[16]{}, model_name[31]{}, firmware_revsion[9]{};
        strncpy(serial_number, data, 15);
        strncpy(model_name, data + 20, 30);
        strncpy(firmware_revsion, data + 60, 8);

        printf("[+] Model Name -> %s\n", model_name);
        printf("[+] Serial Number -> %s\n", serial_number);
        printf("[+] Firmware Rev. -> %s\n", firmware_revsion);

        free(query);
    }

    printf("\nFinished looping your drives!\n");
    system("pause");

    return EXIT_SUCCESS;
}
