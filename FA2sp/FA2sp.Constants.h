#pragma once

#define __str(x) __str_(x)
#define __str_(x) #x

#define PRODUCT_MAJOR 1
#define PRODUCT_MINOR 6
#define PRODUCT_REVISION 3

#define HE_PRODUCT_MAJOR 1
#define HE_PRODUCT_MINOR 2
#define HE_PRODUCT_REVISION 1

#ifdef NDEBUG
#define PRODUCT_STR __str(PRODUCT_MAJOR) "." __str(PRODUCT_MINOR) "." __str(PRODUCT_REVISION)
#define HE_PRODUCT_STR HE_PRODUCT_MAJOR.HE_PRODUCT_MINOR.HE_PRODUCT_REVISION
#else
#define PRODUCT_STR __DATE__ " " __TIME__
#define HE_PRODUCT_STR __DATE__ " " __TIME__
#endif

#define HDM_PRODUCT_VERSION HDM Edition HE_PRODUCT_STR
#define PROGRAM_TITLE FinalAlert 2 SP: HDM_PRODUCT_VERSION
#define DISPLAY_STR PRODUCT_STR
#define PRODUCT_NAME "FA2sp"
#define VERSION_STRVER PRODUCT_NAME " " PRODUCT_STR

#define LOADING_VERSION "Final Alert 2 1.02 - " VERSION_STRVER
#define LOADING_AUTHOR "FA2 by Matthias Wagner - FA2sp by secsome"
#define LOADING_WEBSITE "Github Repository: https://github.com/secsome/FA2sp"

#define APPLY_INFO "Found Final Alert 2 version 1.02. Applying " VERSION_STRVER ", " __str(HDM_PRODUCT_VERSION)

#define MUTEX_HASH_VAL "b8097bca8590a4f46c975ebb43503aab2243ce7f1c87f12f7984dbe1"
#ifdef CHINESE
#define MUTEX_INIT_ERROR_MSG "�����Ѿ���������Щ���ܿ��ܲ�����ȷ������ȷ��Ҫ������"
#define MUTEX_INIT_ERROR_TIT "FA2sp ���������"
#else
#define MUTEX_INIT_ERROR_MSG "The program has already launched! Some function may work not correctly. Do you still want to launch it?"
#define MUTEX_INIT_ERROR_TIT "FA2sp Init Checker"
#endif


