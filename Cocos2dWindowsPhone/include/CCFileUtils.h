/*
* cocos2d-x   http://www.cocos2d-x.org
*
* Copyright (c) 2010-2011 - cocos2d-x community
* 
* Portions Copyright (c) Microsoft Open Technologies, Inc.
* All Rights Reserved
* 
* Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at 
* 
* http://www.apache.org/licenses/LICENSE-2.0 
* 
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an 
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and limitations under the License.
*/

#ifndef __CC_FILEUTILS_PLATFORM_H__
#define __CC_FILEUTILS_PLATFORM_H__

#include <string>
#include "CCDictionary.h"

NS_CC_BEGIN;


//! @brief  Helper class to handle file operations
class CC_DLL CCFileUtils
{
public:
	static CCFileUtils* sharedFileUtils();
	bool init();
	/**
	@brief   Check resource exist or not.
	@return  If the resource exist, return true.
	*/
	static bool isFileExist(const char * resPath);

	/**
	@brief Get resource file data
	@param[in]  pszFileName The resource file name which contain the path
	@param[in]  pszMode The read mode of the file
	@param[out] pSize If get the file data succeed the it will be the data size,or it will be 0
	@return if success,the pointer of data will be returned,or NULL is returned
	@warning If you get the file data succeed,you must delete it after used.
	*/
	static unsigned char* getFileData(const char* pszFileName, const char* pszMode, unsigned long * pSize);
	static unsigned char* getFileDataPlatform(const char* pszFileName, const char* pszMode, unsigned long * pSize);
	static void purgeCachedFileData();

	/**
	@brief Get resource file data from zip file
	@param[in]  pszFileName The resource file name which contain the relative path of zip file
	@param[out] pSize If get the file data succeed the it will be the data size,or it will be 0
	@return if success,the pointer of data will be returned,or NULL is returned
	@warning If you get the file data succeed,you must delete it after used.
	*/
	static unsigned char* getFileDataFromZip(const char* pszZipFilePath, const char* pszFileName, unsigned long * pSize);

	/** removes the suffix from a path
	* On RetinaDisplay it will remove the -hd suffix
	* On iPad it will remove the -ipad suffix
	* On iPhone it will remove the (empty) suffix
	Only valid on iOS. Not valid for OS X.

	@since v0.99.5
	*/
	static std::string& removeSuffixFromFile(std::string& path);


	 /** 
     *  Sets the filenameLookup dictionary.
     *
     *  @param pFilenameLookupDict The dictionary for replacing filename.
     *  @since v2.1
     */
    void setFilenameLookupDictionary(CCDictionary* pFilenameLookupDict);
	void loadFilenameLookupDictionaryFromFile(const char* filename);
	/**
	@brief   Generate the absolute path of the file.
	@param   pszRelativePath     The relative path of the file.
	@return  The absolute path of the file.
	@warning We only add the ResourcePath before the relative path of the file.
	If you have not set the ResourcePath,the function add "/NEWPLUS/TDA_DATA/UserData/" as default.
	You can set ResourcePath by function void setResourcePath(const char *pszResourcePath);
	*/
	static const char* fullPathFromRelativePath(const char *pszRelativePath);

	/** Returns the fullpath of an filename including the resolution of the image.

	If in RetinaDisplay mode, and a RetinaDisplay file is found, it will return that path.
	If in iPad mode, and an iPad file is found, it will return that path.

	Examples:

	* In iPad mode: "image.png" -> "/full/path/image-ipad.png" (in case the -ipad file exists)
	* In RetinaDisplay mode: "image.png" -> "/full/path/image-hd.png" (in case the -hd file exists)

	If an iPad file is found, it will set resolution type to kCCResolutioniPad
	If a RetinaDisplay file is found, it will set resolution type to kCCResolutionRetinaDisplay

	*/
	static const char* fullPathFromRelativePath(const char *pszRelativePath, ccResolutionType *pResolutionType);

	/// @cond
	static const char* fullPathFromRelativeFile(const char *pszFilename, const char *pszRelativeFile);
	/// @endcond

	/** Sets the iPhone RetinaDisplay suffix to load resources.
	By default it is "-hd".
	Only valid on iOS. Not valid for OS X.

	@since v1.1
	*/
	static void setiPhoneRetinaDisplaySuffix(const char *suffix);

	/** Sets the iPad suffix to load resources.
	By default it is "".
	Only valid on iOS. Not valid for OS X.


	*/
	static void setiPadSuffix(const char *suffix);

	/** Sets the iPad Retina Display suffix to load resources.
	By default it is "-ipadhd".
	Only valid on iOS. Not valid for OS X.

	@since v1.1
	*/
	static void setiPadRetinaDisplaySuffix(const char *suffix);


	std::string getNewFilename(const char* pszFileName);
    /** Dictionary used to lookup filenames based on a key.
     It is used internally by the following methods:
     
     const char* fullPathForFilename(const char* )key;
     
     @since v2.1
     */
    CCDictionary* m_pFilenameLookupDict;
	/** Returns whether or not a given filename exists with the iPad suffix.
	Only available on iOS. Not supported on OS X.
	@since v1.1
	*/
	bool iPadFileExistsAtPath(const char *filename);

	/** Returns whether or not a given filename exists with the iPad RetinaDisplay suffix.
	Only available on iOS. Not supported on OS X.

	*/
	bool iPadRetinaDisplayFileExistsAtPath(const char *filename);

	/** Returns whether or not a given path exists with the iPhone RetinaDisplay suffix.
	Only available on iOS. Not supported on OS X.
	@since v1.1
	*/
	bool iPhoneRetinaDisplayFileExistsAtPath(const char *filename);
	void setSearchPaths(const std::vector<std::string>& searchPaths);

	 /**
     *  Destroys the instance of CCFileUtils.
     */
    static void purgeFileUtils();
        /**
     *  Purges the file searching cache.
     *
     *  @note It should be invoked after the resources were updated.
     *        For instance, in the CocosPlayer sample, every time you run application from CocosBuilder,
     *        All the resources will be downloaded to the writable folder, before new js app launchs,
     *        this method should be invoked to clean the file search cache.
     */
    void purgeCachedEntries();
    /**
     *  Gets the array of search paths.
     *  
     *  @return The array of search paths.
     *  @see fullPathForFilename().
     */
    const std::vector<std::string>& getSearchPaths();
       /**
     @brief  Set the resource directory; we will find resources relative to this directory.
     @param pszDirectoryName  Relative path to root.
     @deprecated Please use setSearchPaths instead.
    */
    CC_DEPRECATED_ATTRIBUTE void setResourceDirectory(const char *pszDirectoryName);
	/**
	@brief  Set the ResourcePath,we will find resource in this path
	@param pszResourcePath  The absolute resource path
	@warning Don't call this function in android and iOS, it has not effect.
	In android, if you want to read file other than apk, you shoud use invoke getFileData(), and pass the 
	absolute path.
	*/
	void setSearchResolutionsOrder(const std::vector<std::string>& searchResolutionsOrder);
    
    /**
     *  Gets the array that contains the search order of the resources.
     *
     *  @see setSearchResolutionsOrder(), fullPathForFilename().
     *  @since v2.1
     */
    const std::vector<std::string>& getSearchResolutionsOrder();

	static void setResourcePath(const char *pszResourcePath);
	std::string fullPathForFilename(const char* pszFileName);
	std::string getPathForFilename(const std::string& filename, const std::string& resourceDirectory, const std::string& searchPath);
	/**
	@brief   Generate a CCDictionary pointer by file
	@param   pFileName  The file name of *.plist file
	@return  The CCDictionary pointer generated from the file
	*/
	static CCDictionary *dictionaryWithContentsOfFile(const char *pFileName);

	/**
	@brief The same meaning as dictionaryWithContentsOfFile(), but it doesn't call autorelease, so the
	invoker should call release().
	*/
	static CCDictionary *dictionaryWithContentsOfFileThreadSafe(const char *pFileName);

	/**
	@brief   Generate a CCMutableArray pointer by file
	@param   pFileName  The file name of *.plist file
	@return  The CCArray pointer generated from the file
	*/
	static CCArray* arrayWithContentsOfFile(const char* pFileName);

	/*
	@brief The same meaning as arrayWithContentsOfFile(), but it doesn't call autorelease, so the
	invoker should call release().
	*/
	static CCArray* arrayWithContentsOfFileThreadSafe(const char* pFileName);
	/**
	@brief   Get the writeable path
	@return  The path that can write/read file
	*/
	static std::string getWriteablePath();

	/**
	@brief Set/Get whether pop-up a message box when the image load failed
	*/
	static void setPopupNotify(bool bNotify);
	static bool IsPopupNotify();

	///////////////////////////////////////////////////
	// interfaces on wophone
	///////////////////////////////////////////////////
	/**
	@brief  Set the resource zip file name
	@param pszZipFileName The relative path of the .zip file
	*/
	static void setResource(const char* pszZipFileName);

	//   ///////////////////////////////////////////////////
	//   // interfaces on ios
	//   ///////////////////////////////////////////////////
	//   static int ccLoadFileIntoMemory(const char *filename, unsigned char **out);
	std::vector<std::string> m_searchResolutionsOrderArray;
	std::vector<std::string> m_searchPathArray;
	std::string m_strDefaultResRootPath;
	std::string m_obDirectory;
};

class CCFileData
{
public:
	CCFileData(const char* pszFileName, const char* pszMode)
		: m_pBuffer(0)
		, m_uSize(0)
	{
		m_pBuffer = CCFileUtils::getFileData(pszFileName, pszMode, &m_uSize);
	}
	~CCFileData()
	{
		CC_SAFE_DELETE_ARRAY(m_pBuffer);
	}

	bool reset(const char* pszFileName, const char* pszMode)
	{
		CC_SAFE_DELETE_ARRAY(m_pBuffer);
		m_uSize = 0;
		m_pBuffer = CCFileUtils::getFileData(pszFileName, pszMode, &m_uSize);
		return (m_pBuffer) ? true : false;
	}

	CC_SYNTHESIZE_READONLY(unsigned char *, m_pBuffer, Buffer);
	CC_SYNTHESIZE_READONLY(unsigned long ,  m_uSize,   Size);
};

NS_CC_END;

#endif	// end of __CC_EGLVIEW_PLATFORM_H__
