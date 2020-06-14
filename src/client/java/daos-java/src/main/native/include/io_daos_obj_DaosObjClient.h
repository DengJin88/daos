/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class io_daos_obj_DaosObjClient */

#ifndef _Included_io_daos_obj_DaosObjClient
#define _Included_io_daos_obj_DaosObjClient
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    encodeObjectId
 * Signature: (JILjava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_io_daos_obj_DaosObjClient_encodeObjectId
  (JNIEnv *, jclass, jlong, jint, jstring, jint);

/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    openObject
 * Signature: (JJI)J
 */
JNIEXPORT jlong JNICALL Java_io_daos_obj_DaosObjClient_openObject
  (JNIEnv *, jobject, jlong, jlong, jint);

/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    closeObject
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_io_daos_obj_DaosObjClient_closeObject
  (JNIEnv *, jobject, jlong);

/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    punchObject
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_io_daos_obj_DaosObjClient_punchObject
  (JNIEnv *, jobject, jlong, jlong);

/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    punchObjectDkeys
 * Signature: (JJIJI)V
 */
JNIEXPORT void JNICALL Java_io_daos_obj_DaosObjClient_punchObjectDkeys
  (JNIEnv *, jobject, jlong, jlong, jint, jlong, jint);

/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    punchObjectAkeys
 * Signature: (JJIJI)V
 */
JNIEXPORT void JNICALL Java_io_daos_obj_DaosObjClient_punchObjectAkeys
  (JNIEnv *, jobject, jlong, jlong, jint, jlong, jint);

/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    queryObjectAttribute
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_io_daos_obj_DaosObjClient_queryObjectAttribute
  (JNIEnv *, jobject, jlong);

/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    fetchObject
 * Signature: (JJIJJ)V
 */
JNIEXPORT void JNICALL Java_io_daos_obj_DaosObjClient_fetchObject
  (JNIEnv *, jobject, jlong, jlong, jint, jlong, jlong);

/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    updateObject
 * Signature: (JJIJJ)V
 */
JNIEXPORT void JNICALL Java_io_daos_obj_DaosObjClient_updateObject
  (JNIEnv *, jobject, jlong, jlong, jint, jlong, jlong);

/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    listObjectDkeys
 * Signature: (JJJJI)V
 */
JNIEXPORT void JNICALL Java_io_daos_obj_DaosObjClient_listObjectDkeys
  (JNIEnv *, jobject, jlong, jlong, jlong, jlong, jint);

/*
 * Class:     io_daos_obj_DaosObjClient
 * Method:    listObjectAkeys
 * Signature: (J[BJJJI)V
 */
JNIEXPORT void JNICALL Java_io_daos_obj_DaosObjClient_listObjectAkeys
  (JNIEnv *, jobject, jlong, jbyteArray, jlong, jlong, jlong, jint);

#ifdef __cplusplus
}
#endif
#endif
