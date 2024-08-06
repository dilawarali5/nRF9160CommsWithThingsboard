/**
 * @file storage.h
 * @brief Flash storage APIs. 
 * 
 * @copyright Copyright (C) A9S Inc. - All Rights Reserved.
 Unauthorized copying of this file, via any medium is strictly prohibited.
 Proprietary and confidential. * 
 */


#ifndef Storage_h
#define Storage_h

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum file name length in little FS */
#define STORAGE_FILE_MAX_PATH_LEN 255
#define INPUT_NAME_MAX_LENGTH     245

/*Tags to use for return data of files list*/
#define START_BYTE          0x0E
#define END_BYTE            0x0F
#define TOTAL_FILES_TAG     0x10
#define BLOB_FILE_TAG       0x11

/*API ERROR Codes*/
#define ERROR_FILE_LENGTH_NOT_SUPPORTED     4000


/* Directories for sorage */
#define DIRECTORY "st"

/* Flag to enable/disable Mutex Locking in storage api*/
#define FLASH_STORAGE_MUTEX_LOCK_ENABLED        1

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
int32_t read_file(const uint8_t *filename, uint8_t *data, uint32_t data_size, const uint8_t* directory);

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
int32_t write_file(const uint8_t *filename, const uint8_t *data, uint32_t data_size, const uint8_t* directory);

/**@brief 				Function to remove the Blob information from file system
 *
 * @details 			Delete the file in file system to remove blob data referencing provided key value
 *
 * @param[in]	 		name				filename to erase.
 * @param[in]	 		directory			Char array to provide target directory
 * @param[out]   		int32_t				returns 0 for success and negative ERROR code incase of an error.
 */
int32_t eraseFile(uint8_t *name, uint8_t* directory);

/**@brief 				Function to list the Blob files available in file system
 *
 * @details 			List all the files name available int the file system. Indicating the available Blobs.
 *
 * @param[in]	 		FileListBuffer		Char array to store the files name. File names will be stored in TLV format
 * @param[in]	 		directory			Char array to provide target directory
 * @param[out]   		int32_t				returns the number of bytes written in buffer or negative ERROR code incase of an error.
 */
int32_t listBlobFiles(uint8_t *FileListBuffer, uint8_t* directory);
int32_t EraseExternalFlash();

#ifdef __cplusplus
}
#endif

#endif