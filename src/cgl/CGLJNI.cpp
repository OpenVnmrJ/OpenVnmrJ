/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
*/
// Java Native interface (CGLRenderer.java )
#include "vnmr_jgl_CGLJNI.h"      // auto-generated JNI header
#include "CGLTexMgr.h" // C++ class header file
#include "CGLRenderer.h" // C++ class header file

#include <iostream>

//#define DEBUG_NATIVE

static CGLRenderer *getRenderer(JNIEnv * env, jobject obj){
	jclass cls = env->GetObjectClass(obj);
	jfieldID fid = env->GetFieldID(cls, "native_obj", "J");
    jlong x = env->GetLongField(obj, fid);
#ifdef DEBUG_GETRENDERER
    std::cout << "CGLJNI getRenderer("<< (CGLRenderer*)x << ")" << std::endl;
#endif
	if(x==0)
		std::cout << "CGLJNI ERROR getRenderer("<< (CGLRenderer*)x << ")" << std::endl;
    return (CGLRenderer*)x;
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCcreate
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_vnmr_jgl_CGLJNI_CCcreate(JNIEnv * e, jobject c)
{
    CGLRenderer *obj=new CGLRenderer();
#ifdef DEBUG_NATIVE
    std::cout << "CGLJNI create("<< obj << ")" << "jlong:"<< sizeof(jlong) << " ptr:" << sizeof(obj) << std::endl;
#endif
    return (jlong)obj;
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCdestroy
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCdestroy
  (JNIEnv *e, jobject c, jlong obj)
{
#ifdef DEBUG_NATIVE
      std::cout << "CGLJNI destroy("<< obj << ")" << std::endl;
#endif
	  CGLRenderer *test=(CGLRenderer*)obj;
	  if(test)
	  delete test;
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCinit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCinit(JNIEnv * e, jobject c, jint f)
{
	CGLRenderer *renderer=getRenderer(e,c);
#ifdef DEBUG_NATIVE
    std::cout << "CGLJNI init()" << std::endl;
#endif
	if(renderer)
		renderer->init(f);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCrender
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCrender(JNIEnv * e, jobject c, jint f)
{
	CGLRenderer *renderer=getRenderer(e,c);
#ifdef DEBUG_NATIVE
    std::cout << "CGLJNI render()" << std::endl;
#endif
	if(renderer)
		renderer->render(f);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetOptions
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetOptions
  (JNIEnv *e, jobject c, jint indx, jint s)
{
#ifdef DEBUG_NATIVE
      std::cout << "CGLJNI setOptions("<<indx<<","<<s<<")"<< std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setOptions(indx, s);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCresize
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCresize(JNIEnv* e, jobject c, jint w, jint h)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI resize(" << w << "," << h << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->resize(w,h);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetColorArray
 * Signature: (I[F)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetColorArray
  (JNIEnv *e, jobject c, jint id, jfloatArray p, jint n)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setColorArray(" << id << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer){
		jboolean iscopy=false;
		void *data=e->GetPrimitiveArrayCritical(p,&iscopy);
		renderer->setColorArray(id, (float*)data, n);
		e->ReleasePrimitiveArrayCritical(p, (float*)data, 0);
	}
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetData
 * Signature: ([FIIIIFF)V
 *
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetDataPars
  (JNIEnv *e, jobject c, jint n, jint t, jint s, jint dtype)
{
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer){
#ifdef DEBUG_NATIVE
		std::cout << "CGLJNI setDataPars(" << n << ","<< t << ","<< s << ","<< dtype <<")" << std::endl;
#endif
		renderer->setDataPars(n,t,s,dtype);
	}
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetDataMap
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetDataMap
  (JNIEnv *e, jobject c, jstring s)
{
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer){
		jboolean iscopy;
		const char *mfile = e->GetStringUTFChars(s, &iscopy);
#ifdef DEBUG_NATIVE
		std::cout << "CGLJNI setDataMap(" << mfile <<")" << std::endl;
#endif
		renderer->setDataMap((char*)mfile);
		e->ReleaseStringUTFChars(s, mfile);
	}
}


/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetDataPtr
 * Signature: ([F)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetDataPtr
  (JNIEnv *e, jobject c, jfloatArray p)
{
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer){
		if(p >0){
			jboolean iscopy=false;
			void *data=e->GetPrimitiveArrayCritical(p,&iscopy);
			if(iscopy)
			    std::cout<< "CGLJNI setData("<< data <<")" << " copy" << std::endl;
#ifdef DEBUG_NATIVE
			std::cout << "CGLJNI setData(" << (float*)data  <<")" << std::endl;
#endif
			renderer->setDataPtr((float*)data);
		}
		else{
#ifdef DEBUG_NATIVE
		std::cout << "CGLJNI setData(" << 0  <<")" << std::endl;
#endif
			renderer->setDataPtr((float*)0);
		}
	}
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCreleaseData
 * Signature: ([F)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCreleaseDataPtr(JNIEnv *e, jobject c, jfloatArray p)
{
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer){
		jfloat *data=renderer->getDataPtr();
		if(p >0)
			e->ReleasePrimitiveArrayCritical(p, data, 0);
#ifdef DEBUG_NATIVE
		std::cout << "CGLJNI releaseDataPtr("<< data << ")" << std::endl;
#endif
	}
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetDataScale
 * Signature: (DD)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetDataScale
  (JNIEnv *e, jobject c, jdouble mn, jdouble mx)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setDataScale("<< mn << ","  << mx << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setDataScale(mn,mx);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetPhase
 * Signature: (DD)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetPhase
(JNIEnv *e, jobject c, jdouble r , jdouble l)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setPhase(" << r << "," << l << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setPhase(r,l);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetScale
 * Signature: (DDD)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetScale
 (JNIEnv *e, jobject c, jdouble a, jdouble x, jdouble y, jdouble z)
 {
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setScale("<< a << "," << x << "," << y << "," << z << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setScale(a,x,y,z);
 }

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetSpan
 * Signature: (DDD)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetSpan
 (JNIEnv *e, jobject c, jdouble x, jdouble y, jdouble z)
 {
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setSpan("<< x << "," << y << "," << z << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setSpan(x,y,z);
 }

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetOffset
 * Signature: (DDD)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetOffset
 (JNIEnv *e, jobject c, jdouble x, jdouble y, jdouble z)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setOffset(" << x << "," << y << "," << z << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setOffset(x,y,z);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetRotation3D
 * Signature: (DDD)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetRotation2D
 (JNIEnv *e, jobject c, jdouble x, jdouble y)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setRotation2D(" << x << "," << y << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setRotation2D(x,y);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetRotation3D
 * Signature: (DDD)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetRotation3D
 (JNIEnv *e, jobject c, jdouble x, jdouble y, jdouble z)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setRotation3D(" << x << "," << y << "," << z << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setRotation3D(x,y,z);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetObjectRotation
 * Signature: (DDD)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetObjectRotation
 (JNIEnv *e, jobject c, jdouble x, jdouble y, jdouble z)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setObjectRotation(" << x << "," << y << "," << z << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setObjectRotation(x,y,z);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetTrace
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetTrace
  (JNIEnv *e, jobject c, jint i, jint max,jint num)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setTrace(" << i << "," << max << "," << num  << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setTrace(i,max,num);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetSlice
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetSlice
  (JNIEnv *e, jobject c, jint i,jint max,jint num)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setSlice(" << i << "," << max << "," << num  << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setSlice(i,max,num);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    render2DPoint
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCrender2DPoint
  (JNIEnv *e, jobject c, jint pt, jint trc, jint dtype)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI CCrender2DPoint(" << pt << "," << trc << "," << dtype  << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->render2DPoint(pt,trc,dtype);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetStep
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetStep
  (JNIEnv *e, jobject c, jint i)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setStep(" << i << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setStep(i);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetSlant
 * Signature: (DD)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetSlant
  (JNIEnv *e, jobject c, jdouble x, jdouble y)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setSlant(" << x << "," << y")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setSlant(x,y);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetIntensity
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetIntensity
  (JNIEnv *e, jobject c, jdouble x)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI Intensity(" << x << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setIntensity(x);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetBias
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetBias
  (JNIEnv *e, jobject c, jdouble x)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI Bias(" << x << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setBias(x);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetContrast
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetContrast
  (JNIEnv *e, jobject c, jdouble x)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI Contrast(" << x << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setContrast(x);
}
/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetTheshold
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetThreshold
  (JNIEnv *e, jobject c, jdouble x)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI Threshold(" << x << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setThreshold(x);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetContours
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetContours
  (JNIEnv *e, jobject c, jdouble x)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI Contours(" << x << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setContours(x);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetTheshold
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetLimit
  (JNIEnv *e, jobject c, jdouble x)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI Limit(" << x << ")" << std::endl;
#endif
	CGLRenderer *renderer=getRenderer(e,c);
	if(renderer)
		renderer->setLimit(x);
}
/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetTransparency
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetTransparency
  (JNIEnv *e, jobject c, jdouble x)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI Transparency(" << x << ")" << std::endl;
#endif
    CGLRenderer *renderer=getRenderer(e,c);
    if(renderer)
        renderer->setTransparency(x);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetAlphaScale
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetAlphaScale
  (JNIEnv *e, jobject c, jdouble x)
{
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI AlphaScale(" << x << ")" << std::endl;
#endif
    CGLRenderer *renderer=getRenderer(e,c);
    if(renderer)
        renderer->setAlphaScale(x);
}

/*
 * Class:     vnmr_jgl_CGLJNI
 * Method:    CCsetSliceVector
 * Signature: (DDD)V
 */
JNIEXPORT void JNICALL Java_vnmr_jgl_CGLJNI_CCsetSliceVector
 (JNIEnv *e, jobject c, jdouble x, jdouble y, jdouble z,jdouble w)
 {
#ifdef DEBUG_NATIVE
     std::cout << "CGLJNI setSliceVector(" << x << "," << y << "," << z << "," << w << ")" << std::endl;
#endif
    CGLRenderer *renderer=getRenderer(e,c);
    if(renderer)
        renderer->setSliceVector(x,y,z,w);
 }
