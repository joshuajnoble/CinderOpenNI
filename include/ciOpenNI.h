//
//  OpenNI.h
//  OpenNI
//
//

#pragma once

#include <OpenNI.h>

#ifdef USING_NITE

#include <NiTE.h>
#include <OniCTypes.h>
#include "NiteCEnums.h"

#endif

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
    
    
    class ImageSourceRGBDColor : public ImageSource 
    {
    public:
        ImageSourceRGBDColor( uint8_t *buffer, int width, int height )
        : ImageSource(), mData( buffer ), _width(width), _height(height)
        {
            setSize( _width, _height );
            setColorModel( ImageIo::CM_RGB );
            setChannelOrder( ImageIo::RGB );
            setDataType( ImageIo::UINT8 );
        }
        
        ~ImageSourceRGBDColor()
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
    
    
    class ImageSourceRGBDDepth : public ImageSource 
    {
    public:
        ImageSourceRGBDDepth( uint16_t *buffer, int width, int height )
        : ImageSource(), mData( buffer ), _width(width), _height(height)
        {
            setSize( _width, _height );
            setColorModel( ImageIo::CM_GRAY );
            setChannelOrder( ImageIo::Y );
            setDataType( ImageIo::UINT16 );
        }
        
        ~ImageSourceRGBDDepth()
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

	class ImageSourceRGBDIR : public ImageSource 
    {
    public:
        ImageSourceRGBDIR( uint16_t *buffer, int width, int height )
        : ImageSource(), mData( buffer ), _width(width), _height(height)
        {
            setSize( _width, _height );
            setColorModel( ImageIo::CM_GRAY );
            setChannelOrder( ImageIo::Y );
            setDataType( ImageIo::UINT16 );
        }
        
        ~ImageSourceRGBDIR()
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
    
    inline cinder::ImageSourceRef fromOpenNI ( openni::VideoFrameRef &frame) {
		if(frame.getSensorType() == openni::SENSOR_COLOR) {
			return cinder::ImageSourceRef( new cinder::ImageSourceRGBDColor( (uint8_t*) frame.getData(), (int) frame.getWidth(), (int) frame.getHeight() ) );
		} else if(frame.getSensorType() == openni::SENSOR_DEPTH) {
			return cinder::ImageSourceRef( new cinder::ImageSourceRGBDDepth( (uint16_t*) frame.getData(), (int) frame.getWidth(), (int) frame.getHeight() ) );
		} else {
			return cinder::ImageSourceRef( new cinder::ImageSourceRGBDIR( (uint16_t*) frame.getData(), (int) frame.getWidth(), (int) frame.getHeight() ) );
		}
    }
    
    inline cinder::ImageSourceRef fromOpenNI (const openni::VideoFrameRef &frame ) {
        
        if(frame.getSensorType() == openni::SENSOR_COLOR) {
			return cinder::ImageSourceRef( new cinder::ImageSourceRGBDColor( (uint8_t*) frame.getData(), (int) frame.getWidth(), (int) frame.getHeight() ) );
		} else if(frame.getSensorType() == openni::SENSOR_DEPTH) {
			return cinder::ImageSourceRef( new cinder::ImageSourceRGBDDepth( (uint16_t*) frame.getData(), (int) frame.getWidth(), (int) frame.getHeight() ) );
		} else {
			return cinder::ImageSourceRef( new cinder::ImageSourceRGBDIR( (uint16_t*) frame.getData(), (int) frame.getWidth(), (int) frame.getHeight() ) );
		}
    }
    
#ifdef USING_NITE

	inline cinder::Vec3f fromOpenNI ( const nite::Point3f nPoint ) {
        return Vec3f(nPoint.x, nPoint.y, nPoint.z);
    }

    inline cinder::Vec3f fromOpenNI ( const NitePoint3f nPoint ) {
        return Vec3f(nPoint.x, nPoint.y, nPoint.z);
    }
    
    inline nite::Point3f toOpenNI ( const cinder::Vec3f vector ) {
        nite::Point3f np;
        np.x = vector.x;
        np.y = vector.y;
        np.z = vector.z;
        return np;
    }
   
    class CiPlane3D {
    public:
        
        Vec3f normal;
        Vec3f point;
        
        CiPlane3D(nite::Plane plane) {
            normal = fromOpenNI(plane.normal);
            point = fromOpenNI( plane.point);
        }
        
        CiPlane3D() {
        }
    };

#endif 

    //////////////////////////////////////////////////////////////////////
    
 #ifdef WIN32
    //RGB888Pixel
    void YUV422ToRGB888(const uint8_t* pYUVImage, uint8_t * pRGBAImage, uint32_t nYUVSize, uint32_t nRGBSize)
    {
        const uint8_t* pYUVLast = pYUVImage + nYUVSize - 8;
        uint8_t* pRGBLast = pRGBAImage + nRGBSize - 16;
        
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
        
        uint32_t* piR = (uint32_t*)&iR;
        uint32_t* piG = (uint32_t*)&iG;
        uint32_t* piB = (uint32_t*)&iB;
        
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
    
    void YUV444ToRGBA(uint8_t cY, uint8_t cU, uint8_t cV,
                      uint8_t& cR, uint8_t& cG, uint8_t& cB, uint8_t& cA)
    {
        uint32_t nC = cY - 16;
        uint16_t nD = cU - 128;
        uint16_t nE = cV - 128;
        
        nC = nC * 298 + 128;
        
        cR = XN_MIN(XN_MAX((nC            + 409 * nE) >> 8, 0), 255);
        cG = XN_MIN(XN_MAX((nC - 100 * nD - 208 * nE) >> 8, 0), 255);
        cB = XN_MIN(XN_MAX((nC + 516 * nD           ) >> 8, 0), 255);
        cA = 255;
    }
    
    void YUV422ToRGB888(const uint8_t* pYUVImage, uint8_t* pRGBImage, uint32_t nYUVSize, uint32_t nRGBSize)
    {
        const uint8_t* pCurrYUV = pYUVImage;
        uint8_t* pCurrRGB = pRGBImage;
        const uint8_t* pLastYUV = pYUVImage + nYUVSize - YUV422_BPP;
        uint8_t* pLastRGB = pRGBImage + nRGBSize - YUV_RGBA_BPP;
        
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


