/**
 * @file storage.c
 * @brief Flash storage APIs. 
 * 
 * @copyright Copyright (C) A9S Inc. - All Rights Reserved.
 Unauthorized copying of this file, via any medium is strictly prohibited.
 Proprietary and confidential. * 
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/sys/printk.h>
#include "storage.h"

/* File System configuratoin */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *)FLASH_AREA_ID(littlefs_storage),
	.mnt_point = "/lfs",
};

/* File system mount point and other information*/
static struct fs_mount_t *littleFsMountInfo = &lfs_storage_mnt;

/* External Flash storage Mutex to synchronize */
K_MUTEX_DEFINE(StorageMutex);

#define APP_CORE_DFU_PARTITION_ID FIXED_PARTITION_ID(slot1_partition)
#define NET_CORE_DFU_PARTITION_ID FIXED_PARTITION_ID(slot3_partition)

/**@brief 				Function to initialize required directory
 *
 * @details 			Verify the required directory already exist otherwise create it
 *
 * @param[in]	 		directory		Char array. path of the directory
 * @param[out]   		int32_t			returns 0 on success or negative ERROR code incase of an error.
 */
static int32_t initializeDirectory(const uint8_t* directory)
{
	/*File system path and name buffer*/
	uint8_t directoryName[STORAGE_FILE_MAX_PATH_LEN] = {0};
	int32_t rc = 0;
	struct fs_dirent dirent;

	snprintf(directoryName, STORAGE_FILE_MAX_PATH_LEN, "%s/%s", littleFsMountInfo->mnt_point, directory);

	/*Retrieve directory information*/	
	rc = fs_stat(directoryName, &dirent);
	if (rc == -ENOENT)
	{
		/* Create requested directory */
		rc = fs_mkdir(directoryName);
	}

	return rc;
}


/**@brief 				Function to retrieve the Blob information from file system
 *
 * @details 			Read file system for blob data referencing provided key value
 *
 * @param[in]	 		filename			File to read.
 * @param[in]	 		data				buffer to fill with read data. 
 * @param[in]	 		data_size			Max number of bytes to read. 
 * @param[in]	 		directory			Directory name.
 * @param[out]   		int32_t				returns number of data bytes read from file system. Negative ERROR code incase of an error.
 */
int32_t read_file(const uint8_t *filename, uint8_t *data, uint32_t data_size, const uint8_t* directory) {
	struct fs_dirent dirent;
	struct fs_file_t file;

	/*File system path and name buffer*/
	uint8_t file_path[STORAGE_FILE_MAX_PATH_LEN] = {0};
	int32_t rc;

	if( (strlen(filename) + strlen(directory) + 2) > INPUT_NAME_MAX_LENGTH)
	{
		printk("Provided file name is too long\n");
		return -ERROR_FILE_LENGTH_NOT_SUPPORTED;
	}

	#if FLASH_STORAGE_MUTEX_LOCK_ENABLED == 1
		k_mutex_lock(&StorageMutex, K_FOREVER);
	#endif

	/* File system file path and name*/
	snprintf(file_path, STORAGE_FILE_MAX_PATH_LEN, "%s/%s/%s", littleFsMountInfo->mnt_point, directory, filename);
	printk("File Path: %s\n", file_path);

	/*Mount File system before any operation*/
	rc = fs_mount(littleFsMountInfo);
	if (rc >= 0)
	{
		/*Retrieve Blob information with respect to key*/
		rc = fs_stat(file_path, &dirent);
		if (rc >= 0)
		{
			/*Open file Read only mode*/
			fs_file_t_init(&file);
			rc = fs_open(&file, file_path, FS_O_READ);
			if (rc >= 0)
			{
				rc = fs_read(&file, data, MIN(dirent.size, data_size));
			}

			printk("Read file: %s\n", filename);
			printk("Data: %s\n", data);

			fs_close(&file);
		}
		
		/*Unmount file system before exit*/
		fs_unmount(littleFsMountInfo);
	}

	#if FLASH_STORAGE_MUTEX_LOCK_ENABLED == 1
		k_mutex_unlock(&StorageMutex);
	#endif

	return rc;
}

/**@brief 				Function to write a file.
 *
 * @details 			write file in file system to store blob data referencing provided key value
 *
 * @param[in]	 		filename			File to write.
 * @param[in]	 		data				buffer with data to write. 
 * @param[in]	 		data_size			Number of bytes to write. 
 * @param[in]	 		directory			Directory name.
 * @param[out]   		int32_t				returns number of data bytes written in file system. Negative ERROR code incase of an error.
 */
int32_t write_file(const uint8_t *filename, const uint8_t *data, uint32_t data_size, const uint8_t* directory)
{
	struct fs_file_t file;
	uint8_t file_path[STORAGE_FILE_MAX_PATH_LEN] = {0};
	int32_t rc;

	if( (strlen(filename) + strlen(directory) + 2) > INPUT_NAME_MAX_LENGTH)	{
		printk("Provided file name is too long\n");
		return -ERROR_FILE_LENGTH_NOT_SUPPORTED;
	}

	#if FLASH_STORAGE_MUTEX_LOCK_ENABLED == 1
		k_mutex_lock(&StorageMutex, K_FOREVER);
	#endif

	/* File system file path and name*/
	snprintf(file_path, STORAGE_FILE_MAX_PATH_LEN, "%s/%s/%s", littleFsMountInfo->mnt_point, directory, filename);

	/*Mount File system before any operation*/
	rc = fs_mount(littleFsMountInfo);
	if (rc >= 0)
	{
		rc = initializeDirectory(directory);
		if (rc >= 0)
		{
			/*Open file in create and ReadWrite mode. File will be created incase not existed. Existed file data will be over written*/
			fs_file_t_init(&file);
			rc = fs_open(&file, file_path, FS_O_CREATE | FS_O_RDWR);
			if (rc >= 0)
			{
				rc = fs_truncate (&file, 0);
				rc = fs_write(&file, data, data_size);
			}

			printk("Write file: %s\n", filename);
			printk("Data: %s\n", data);

			/*Close file */
			fs_close(&file);
		}

		/*Unmount file system before exit*/
		fs_unmount(littleFsMountInfo);
	}

	#if FLASH_STORAGE_MUTEX_LOCK_ENABLED == 1
		k_mutex_unlock(&StorageMutex);
	#endif
	return rc;
}


/**@brief 				Function to remove the Blob information from file system
 *
 * @details 			Delete the file in file system to remove blob data referencing provided key value
 *
 * @param[in]	 		key					Char array. first item indicating length and rest is key data
 * @param[in]	 		directory			Char array to provide target directory
 * @param[out]   		int32_t				returns 0 for success and negative ERROR code incase of an error.
 */
int32_t eraseBlob(uint8_t *key, uint8_t* directory)
{
	uint8_t keyLength = key[0];
	uint8_t *keyName = &key[1];
	uint8_t fileName[STORAGE_FILE_MAX_PATH_LEN] = {0};
	int32_t rc;
 	struct fs_dirent dirent;

	if( (keyLength + strlen(directory)) > INPUT_NAME_MAX_LENGTH)
	{
		printk("Provided file name is too long\n");
		return -ERROR_FILE_LENGTH_NOT_SUPPORTED;
	}

	#if FLASH_STORAGE_MUTEX_LOCK_ENABLED == 1
		k_mutex_lock(&StorageMutex, K_FOREVER);
	#endif

	/*File system path and name buffer*/
	snprintf(fileName, STORAGE_FILE_MAX_PATH_LEN, "%s/%s/%s", littleFsMountInfo->mnt_point, directory, keyName);

	/*Mount File system before any operation*/
	rc = fs_mount(littleFsMountInfo);
	if (rc >= 0)
	{
		/*Retrieve Blob information with respect to key*/
		rc = fs_stat(fileName, &dirent);
		if (rc >= 0)
		{
			/*Erase the file from file system*/
			rc = fs_unlink(fileName);
		} else if (rc == -ENOENT)
		{
			rc = 0;
		}
		
		/*Unmount file system before exit*/
		fs_unmount(littleFsMountInfo);
	}

	#if FLASH_STORAGE_MUTEX_LOCK_ENABLED == 1
		k_mutex_unlock(&StorageMutex);
	#endif
	return rc;
}

/**@brief 				Function to list the Blob files available in file system
 *
 * @details 			List all the files name available int the file system. Indicating the available Blobs.
 *
 * @param[in]	 		FileListBuffer		Char array to store the files name. File names will be stored in TLV format
 * @param[in]	 		directory			Char array to provide target directory
 * @param[out]   		int32_t				returns the number of bytes written in buffer or negative ERROR code incase of an error.
 */
int32_t listBlobFiles(uint8_t *FileListBuffer, uint8_t* directory)
{
    struct fs_dir_t dir;
    int32_t rc = 0;
	int32_t index = 0;
	uint8_t  totalNumberOfFiles = 0;

	#if FLASH_STORAGE_MUTEX_LOCK_ENABLED == 1
		k_mutex_lock(&StorageMutex, K_FOREVER);
	#endif

	/*File system path and name buffer*/
	uint8_t directoryName[STORAGE_FILE_MAX_PATH_LEN] = {0};

	snprintf(directoryName, STORAGE_FILE_MAX_PATH_LEN, "%s/%s", littleFsMountInfo->mnt_point, directory);

	/*Add start byte at the start of the buffer*/
	FileListBuffer[0] = START_BYTE;
	FileListBuffer[1] = TOTAL_FILES_TAG;
	FileListBuffer[2] = 1;		// Length of Total files value. Max 255
	FileListBuffer[3] = 0;		// Number of file names in buffer. Need to overwrite it later.
	index += 4;

	/*Mount File system before any operation*/
	rc = fs_mount(littleFsMountInfo);
	if (rc >= 0)
	{
		// Open the directory
		fs_dir_t_init(&dir);
		rc = fs_opendir(&dir, directoryName);
		if (rc != 0) {
			printk("FAIL: unable to open the directory \"/lfs\"\n");
			return rc;
		}

		// Read the names of the files in the directory
		struct fs_dirent entry;
		while ((rc = fs_readdir(&dir, &entry)) == 0) 
		{
			/* Stop Reading at the end of directory*/
			if (entry.name[0] == 0){
				break;
			}

			/*Add the Tag for Blob file*/
			FileListBuffer[index++] = BLOB_FILE_TAG;

			/*Copy file name Length in buffer*/
			FileListBuffer[index++] = (uint8_t) strlen(entry.name);

			/*Copy the File name in buffer*/
			memcpy(&FileListBuffer[index], entry.name, strlen(entry.name));
			index += strlen(entry.name);

			printk("File: %s", entry.name);
			/*Count the number of files in directory*/
			totalNumberOfFiles++;
		}

		/* Update the Number of files count in buffer*/
		FileListBuffer[3] = totalNumberOfFiles;

		/*Add buffer data end byte*/
		FileListBuffer[index] = END_BYTE;

		// Close the directory and unmount file system
		fs_closedir(&dir);
		fs_unmount(littleFsMountInfo);
	}

	#if FLASH_STORAGE_MUTEX_LOCK_ENABLED == 1
		k_mutex_unlock(&StorageMutex);
	#endif

    return (rc >= 0)? (index + 1) : rc;
}

/**@brief 				Function to erase the external flash
 *
 * @details 			Erase the external flash storage
 *
 * @param[in]	 		None.
 * @param[out]   		int32_t				returns 0 for success and negative ERROR code incase of an error.
 */
int32_t EraseExternalFlash()
{
	unsigned int id = (uintptr_t)littleFsMountInfo->storage_dev;
	const struct flash_area *pfa;

	int rc = flash_area_open(id, &pfa);
	if (rc < 0)
	{
		printk("FAIL: unable to find flash area %u: %d\n", id, rc);
		return rc;
	}

	printk("Flash area %u: %u bytes\n", id, pfa->fa_size);

	printk("Erasing flash area ... ");
	rc = flash_area_erase(pfa, 0, pfa->fa_size);
	printk("%d\n", rc);

	flash_area_close(pfa);

	return rc;
}