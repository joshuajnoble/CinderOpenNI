//
//  OpenNI.h
//  OpenNI
//
//  Created by frogTemplate on 3/17/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

//#include <XnOpenNI.h>
#include <XnCppWrapper.h>

#include "cinder/Cinder.h"
#include "cinder/imageio.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"


namespace cinder {
    
#define YUV422_U  0
#define YUV422_Y1 1
#define YUV422_V  2
#define YUV422_Y2 3
#define YUV422_BPP 4
#define YUV_RED   0
#define YUV_GREEN 1
#define YUV_BLUE  2
#define YUV_ALPHA  3
#define YUV_RGBA_BPP 4
    
#define MAX_DEPTH = 10000;
#define MIN_DEPTH = -9999;
    
    
    class ImageSourceKinectColor : public ImageSource 
    {
    public:
        ImageSourceKinectColor( uint8_t *buffer, int width, int height )
        : ImageSource(), mData( buffer ), _width(width), _height(height)
        {
            setSize( _width, _height );
            setColorModel( ImageIo::CM_RGB );
            setChannelOrder( ImageIo::RGB );
            setDataType( ImageIo::UINT8 );
        }
        
        ~ImageSourceKinectColor()
        {
            // mData is actually a ref. It's released from the device. 
            /*if( mData ) {
             delete[] mData;
             mData = NULL;
             }*/
        }
        
        void load( ImageTargetRef target )
        {
            ImageSource::RowFunc func = setupRowFunc( target );
            
            for( uint32_t row	 = 0; row < _height; ++row )
                ((*this).*func)( target, row, mData + row * _width * 3 );
        }
        
    protected:
        uint32_t					_width, _height;
        uint8_t						*mData;
    };
    
    
    class ImageSourceKinectDepth : public ImageSource 
    {
    public:
        ImageSourceKinectDepth( uint16_t *buffer, int width, int height )
        : ImageSource(), mData( buffer ), _width(width), _height(height)
        {
            setSize( _width, _height );
            setColorModel( ImageIo::CM_GRAY );
            setChannelOrder( ImageIo::Y );
            setDataType( ImageIo::UINT16 );
        }
        
        ~ImageSourceKinectDepth()
        {
            // mData is actually a ref. It's released from the device. 
            /*if( mData ) {
             delete[] mData;
             mData = NULL;
             }*/
        }
        
        void load( ImageTargetRef target )
        {
            ImageSource::RowFunc func = setupRowFunc( target );
            
            for( uint32_t row = 0; row < _height; ++row )
                ((*this).*func)( target, row, mData + row * _width );
        }
        
    protected:
        uint32_t					_width, _height;
        uint16_t					*mData;
    };
    
    /*
     const XnUInt8*			pImage;
     const XnDepthPixel*		pDepth;
     const XnIRPixel*		pIR;
     
     */
    
    
    inline cinder::ImageSourceRef fromOpenNI (const XnUInt8* &image, xn::ImageMetaData &metaData ) {
        return cinder::ImageSourceRef( new cinder::ImageSourceKinectColor( (uint8_t*) image, (int) metaData.XRes(), (int) metaData.YRes() ) );
    }
    
    inline cinder::ImageSourceRef fromOpenNI ( xn::ImageMetaData &metaData ) {
        return cinder::ImageSourceRef( new cinder::ImageSourceKinectColor( (uint8_t*) metaData.Data(), (int) metaData.XRes(), (int) metaData.YRes() ) );
    }
    
    inline cinder::ImageSourceRef fromOpenNI (const xn::DepthMetaData &metaData ) {
        
        return cinder::ImageSourceRef( new cinder::ImageSourceKinectDepth( (uint16_t*) metaData.Data(), (int) metaData.XRes(), (int) metaData.YRes() ) );
    }
    
    inline cinder::Vec3f fromOpenNI ( const XnVector3D xnVector ) {
        return Vec3f(xnVector.X, xnVector.Y, xnVector.Z);
    }
    
    inline XnVector3D toOpenNI ( const cinder::Vec3f vector ) {
        XnVector3D xv;
        xv.X = vector.x;
        xv.Y = vector.y;
        xv.Z = vector.z;
        return xv;
    }
    
    
    inline XnMatrix3X3 toOpenNI ( const cinder::Matrix33<float> mat ) {
        XnMatrix3X3 xMat;
        xMat.elements[0] = mat.at(0,0);
        xMat.elements[1] = mat.at(1,0);
        xMat.elements[2] = mat.at(2,0);
        xMat.elements[3] = mat.at(0,1);
        xMat.elements[4] = mat.at(1,1);
        xMat.elements[5] = mat.at(2,1);
        xMat.elements[6] = mat.at(0,2);
        xMat.elements[7] = mat.at(1,2);
        xMat.elements[8] = mat.at(2,2);
        return xMat;
    }
    
    inline cinder::Matrix33<float> fromOpenNI ( const XnMatrix3X3 xMat ) {
        cinder::Matrix33<float> ciMat;
        ciMat.set(xMat.elements[0], xMat.elements[1], xMat.elements[2],
                  xMat.elements[3], xMat.elements[4], xMat.elements[4],
                  xMat.elements[6], xMat.elements[7], xMat.elements[8]);
        return ciMat;
    }
    
    
    class CiPlane3D {
    public:
        
        Vec3f normal;
        Vec3f point;
        
        CiPlane3D(XnPlane3D plane) {
            normal = fromOpenNI(plane.vNormal);
            point = fromOpenNI( (XnVector3D) plane.ptPoint);
        }
        
        CiPlane3D() {
            
        }
    };

    //////////////////////////////////////////////////////////////////////
    
    #ifdef WIN32
    
    void YUV422ToRGB888(const XnUInt8* pYUVImage, XnUInt8* pRGBAImage, XnUInt32 nYUVSize, XnUInt32 nRGBSize)
    {
        const XnUInt8* pYUVLast = pYUVImage + nYUVSize - 8;
        XnUInt8* pRGBLast = pRGBAImage + nRGBSize - 16;
        
        const __m128 minus16 = _mm_set_ps1(-16);
        const __m128 minus128 = _mm_set_ps1(-128);
        const __m128 plus113983 = _mm_set_ps1(1.13983F);
        const __m128 minus039466 = _mm_set_ps1(-0.39466F);
        const __m128 minus058060 = _mm_set_ps1(-0.58060F);
        const __m128 plus203211 = _mm_set_ps1(2.03211F);
        const __m128 zero = _mm_set_ps1(0);
        const __m128 plus255 = _mm_set_ps1(255);
        
        // define YUV floats
        __m128 y;
        __m128 u;
        __m128 v;
        
        __m128 temp;
        
        // define RGB floats
        __m128 r;
        __m128 g;
        __m128 b;
        
        // define RGB integers
        __m128i iR;
        __m128i iG;
        __m128i iB;
        
        XnUInt32* piR = (XnUInt32*)&iR;
        XnUInt32* piG = (XnUInt32*)&iG;
        XnUInt32* piB = (XnUInt32*)&iB;
        
        while (pYUVImage <= pYUVLast && pRGBAImage <= pRGBLast)
        {
            // process 4 pixels at once (values should be ordered backwards)
            y = _mm_set_ps(pYUVImage[YUV422_Y2 + YUV422_BPP], pYUVImage[YUV422_Y1 + YUV422_BPP], pYUVImage[YUV422_Y2], pYUVImage[YUV422_Y1]);
            u = _mm_set_ps(pYUVImage[YUV422_U + YUV422_BPP],  pYUVImage[YUV422_U + YUV422_BPP],  pYUVImage[YUV422_U],  pYUVImage[YUV422_U]);
            v = _mm_set_ps(pYUVImage[YUV422_V + YUV422_BPP],  pYUVImage[YUV422_V + YUV422_BPP],  pYUVImage[YUV422_V],  pYUVImage[YUV422_V]);
            
            u = _mm_add_ps(u, minus128); // u -= 128
            v = _mm_add_ps(v, minus128); // v -= 128
            
            /*
             
             http://en.wikipedia.org/wiki/YUV
             
             From YUV to RGB:
             R =     Y + 1.13983 V
             G =     Y - 0.39466 U - 0.58060 V
             B =     Y + 2.03211 U
             
             */ 
            
            temp = _mm_mul_ps(plus113983, v);
            r = _mm_add_ps(y, temp);
            
            temp = _mm_mul_ps(minus039466, u);
            g = _mm_add_ps(y, temp);
            temp = _mm_mul_ps(minus058060, v);
            g = _mm_add_ps(g, temp);
            
            temp = _mm_mul_ps(plus203211, u);
            b = _mm_add_ps(y, temp);
            
            // make sure no value is smaller than 0
            r = _mm_max_ps(r, zero);
            g = _mm_max_ps(g, zero);
            b = _mm_max_ps(b, zero);
            
            // make sure no value is bigger than 255
            r = _mm_min_ps(r, plus255);
            g = _mm_min_ps(g, plus255);
            b = _mm_min_ps(b, plus255);
            
            // convert floats to int16 (there is no conversion to uint8, just to int8).
            iR = _mm_cvtps_epi32(r);
            iG = _mm_cvtps_epi32(g);
            iB = _mm_cvtps_epi32(b);
            
            // extract the 4 pixels RGB values.
            // because we made sure values are between 0 and 255, we can just take the lower byte
            // of each INT16
            pRGBAImage[0] = piR[0];
            pRGBAImage[1] = piG[0];
            pRGBAImage[2] = piB[0];
            pRGBAImage[3] = 255;
            
            pRGBAImage[4] = piR[1];
            pRGBAImage[5] = piG[1];
            pRGBAImage[6] = piB[1];
            pRGBAImage[7] = 255;
            
            pRGBAImage[8] = piR[2];
            pRGBAImage[9] = piG[2];
            pRGBAImage[10] = piB[2];
            pRGBAImage[11] = 255;
            
            pRGBAImage[12] = piR[3];
            pRGBAImage[13] = piG[3];
            pRGBAImage[14] = piB[3];
            pRGBAImage[15] = 255;
            
            // advance the streams
            pYUVImage += 8;
            pRGBAImage += 16;
        }
    }
    
#else // not Win32
    
    void YUV444ToRGBA(XnUInt8 cY, XnUInt8 cU, XnUInt8 cV,
                      XnUInt8& cR, XnUInt8& cG, XnUInt8& cB, XnUInt8& cA)
    {
        XnInt32 nC = cY - 16;
        XnInt16 nD = cU - 128;
        XnInt16 nE = cV - 128;
        
        nC = nC * 298 + 128;
        
        cR = XN_MIN(XN_MAX((nC            + 409 * nE) >> 8, 0), 255);
        cG = XN_MIN(XN_MAX((nC - 100 * nD - 208 * nE) >> 8, 0), 255);
        cB = XN_MIN(XN_MAX((nC + 516 * nD           ) >> 8, 0), 255);
        cA = 255;
    }
    
    void YUV422ToRGB888(const XnUInt8* pYUVImage, XnUInt8* pRGBImage, XnUInt32 nYUVSize, XnUInt32 nRGBSize)
    {
        const XnUInt8* pCurrYUV = pYUVImage;
        XnUInt8* pCurrRGB = pRGBImage;
        const XnUInt8* pLastYUV = pYUVImage + nYUVSize - YUV422_BPP;
        XnUInt8* pLastRGB = pRGBImage + nRGBSize - YUV_RGBA_BPP;
        
        while (pCurrYUV <= pLastYUV && pCurrRGB <= pLastRGB)
        {
            YUV444ToRGBA(pCurrYUV[YUV422_Y1], pCurrYUV[YUV422_U], pCurrYUV[YUV422_V],
                         pCurrRGB[YUV_RED], pCurrRGB[YUV_GREEN], pCurrRGB[YUV_BLUE], pCurrRGB[YUV_ALPHA]);
            pCurrRGB += YUV_RGBA_BPP;
            YUV444ToRGBA(pCurrYUV[YUV422_Y2], pCurrYUV[YUV422_U], pCurrYUV[YUV422_V],
                         pCurrRGB[YUV_RED], pCurrRGB[YUV_GREEN], pCurrRGB[YUV_BLUE], pCurrRGB[YUV_ALPHA]);
            pCurrRGB += YUV_RGBA_BPP;
            pCurrYUV += YUV422_BPP;
        }
    }
    
#endif    
    
    
}

/////////////////////////////////
// need a callback handler here
/////////////////////////////////


