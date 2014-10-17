/*********************************************************************NVMH4****
File:  nv_math.h

Copyright NVIDIA Corporation 2002
TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
*AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS
BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.



Comments:


******************************************************************************/
#ifndef _nv_math_h_
#define _nv_math_h_

#include <assert.h>
#include <math.h>

#ifdef _WIN32
#include <limits>
#else
#include <limits.h>
#endif

#ifdef MACOS
#define sqrtf sqrt
#define sinf sin
#define cosf cos
#define tanf tan
#endif

#include <memory.h>
#include <stdlib.h>
#include <float.h>

namespace nv_math
{

typedef float nv_scalar;

#define nv_zero			      nv_math::nv_scalar(0)
#define nv_one_half             nv_math::nv_scalar(0.5)
#define nv_one			      nv_math::nv_scalar(1.0)
#define nv_two			      nv_math::nv_scalar(2)
#define nv_half_pi            nv_math::nv_scalar(3.14159265358979323846264338327950288419716939937510582 * 0.5)
#define nv_quarter_pi         nv_math::nv_scalar(3.14159265358979323846264338327950288419716939937510582 * 0.25)
#define nv_pi			      nv_math::nv_scalar(3.14159265358979323846264338327950288419716939937510582)
#define nv_two_pi			  nv_math::nv_scalar(3.14159265358979323846264338327950288419716939937510582 * 2.0)
#define nv_oo_pi			  nv_one / nv_pi
#define nv_oo_two_pi	      nv_one / nv_two_pi
#define nv_oo_255   	      nv_one / nv_math::nv_scalar(255)
#define nv_oo_128   	      nv_one / nv_math::nv_scalar(128)
#define nv_to_rad             nv_pi / nv_math::nv_scalar(180)
#define nv_to_deg             nv_math::nv_scalar(180) / nv_pi
#define rad2deg(a)            (a * nv_to_deg)
#define deg2rad(a)            (a * nv_to_rad)
#define nv_eps		          nv_math::nv_scalar(10e-6)
#define nv_double_eps	      nv_math::nv_scalar(10e-6) * nv_two
#define nv_big_eps            nv_math::nv_scalar(10e-2)
#define nv_small_eps          nv_math::nv_scalar(10e-6)
#define nv_sqrthalf           nv_math::nv_scalar(0.7071067811865475244)

#define nv_scalar_max         nv_math::nv_scalar(FLT_MAX)
#define nv_scalar_min         nv_math::nv_scalar(FLT_MIN)

template<class T> struct vector2;
template<class T> struct vector3;
template<class T> struct vector4;
template<class T> struct matrix3;
template<class T> struct matrix4;
template<class T> struct quaternion;

static const nv_scalar array16_id[] =        { nv_one, nv_zero, nv_zero, nv_zero,
                                        nv_zero, nv_one, nv_zero, nv_zero,
                                        nv_zero, nv_zero, nv_one, nv_zero,
                                        nv_zero, nv_zero, nv_zero, nv_one};

static const nv_scalar array16_zero[] =      { nv_zero, nv_zero, nv_zero, nv_zero,
                                        nv_zero, nv_zero, nv_zero, nv_zero,
                                        nv_zero, nv_zero, nv_zero, nv_zero,
                                        nv_zero, nv_zero, nv_zero, nv_zero};

static const nv_scalar array16_scale_bias[] = { nv_one_half, nv_zero,   nv_zero,   nv_zero,
                                         nv_zero,   nv_one_half, nv_zero,   nv_zero,
                                         nv_zero,   nv_zero,   nv_one_half, nv_zero,
                                         nv_one_half, nv_one_half, nv_one_half, nv_one};

static const nv_scalar array9_id[] =         { nv_one, nv_zero, nv_zero,
                                        nv_zero, nv_one, nv_zero,
                                        nv_zero, nv_zero, nv_one};

template<class T>
struct vector2
{
    vector2() { }
    vector2(T x, T y) : x(x), y(y) { }
    vector2(const T* xy) : x(xy[0]), y(xy[1]) { }
    vector2(const vector2& u) : x(u.x), y(u.y) { }
    vector2(const vector3<T>&);

    bool operator==(const vector2 & u) const
    {
        return (u.x == x && u.y == y) ? true : false;
    }

    bool operator!=(const vector2 & u) const
    {
        return !(*this == u );
    }

    vector2 & operator*=(const T & lambda)
    {
        x*= lambda;
        y*= lambda;
        return *this;
    }
    // TL
    vector2 & operator/=(const T & lambda)
    {
        x/= lambda;
        y/= lambda;
        return *this;
    }

    vector2 & operator-=(const vector2 & u)
    {
        x-= u.x;
        y-= u.y;
        return *this;
    }

    vector2 & operator+=(const vector2 & u)
    {
        x+= u.x;
        y+= u.y;
        return *this;
    }

    T & operator[](int i)
    {
        return vec_array[i];
    }

    const T operator[](int i) const
    {
        return vec_array[i];
    }

	T sq_norm() const { return x * x + y * y; }
	T norm() const { return sqrtf(sq_norm()); }

    union {
        struct {
            T x,y;          // standard names for components
        };
        struct {
            T s,t;          // standard names for components
        };
        T vec_array[2];     // array access
    };
};

template<class T>
inline const vector2<T> operator+(const vector2<T>& u, const vector2<T>& v)
{
    return vector2<T>(u.x + v.x, u.y + v.y);
}

template<class T>
inline const vector2<T> operator+(const vector2<T>& u, const T s)
{
    return vector2<T>(u.x + s, u.y + s);
}

template<class T>
inline const vector2<T> operator-(const vector2<T>& u, const vector2<T>& v)
{
    return vector2<T>(u.x - v.x, u.y - v.y);
}

template<class T>
inline const vector2<T> operator-(const vector2<T>& u, const T s)
{
    return vector2<T>(u.x - s, u.y - s);
}

template<class T>
inline const vector2<T> operator*(const T s, const vector2<T>& u)
{
	return vector2<T>(s * u.x, s * u.y);
}

template<class T>
inline const vector2<T> operator*(const vector2<T>& u, const T s)
{
	return vector2<T>(s * u.x, s * u.y);
}

template<class T>
inline const vector2<T> operator/(const vector2<T>& u, const T s)
{
	return vector2<T>(u.x / s, u.y / s);
}

template<class T>
inline const vector2<T> operator*(const vector2<T>&u, const vector2<T>&v)
{
	return vector2<T>(u.x * v.x, u.y * v.y);
}

template<class T> extern vector3<T> & mult(vector3<T>& u, const matrix3<T>& M, const vector3<T>& v);
template<class T> extern vector3<T> & mult(vector3<T>& u, const matrix4<T>& M, const vector3<T>& v);

template<class T>
struct vector3
{
	vector3() { }
    vector3(T x, T y, T z) : x(x), y(y), z(z) { }
    vector3(const T* xyz) : x(xyz[0]), y(xyz[1]), z(xyz[2]) { }
	vector3(const vector2<T>& u) : x(u.x), y(u.y), z(1.0f) { }
	vector3(const vector2<T>& u, T v) : x(u.x), y(u.y), z(v) { }
	vector3(const vector3<T>& u) : x(u.x), y(u.y), z(u.z) { }
	vector3(const vector4<T>&);

    bool operator==(const vector3<T> & u) const
    {
        return (u.x == x && u.y == y && u.z == z) ? true : false;
    }

    bool operator!=( const vector3<T>& rhs ) const
    {
        return !(*this == rhs );
    }

    // TL
    vector3<T> & operator*=(const matrix3<T> & M)
    {
        vector3<T> dst;
        mult(dst, M, *this);
        x = dst.x;
        y = dst.y;
        z = dst.z;
        return (*this);
    }
    // TL
    vector3<T> & operator*=(const matrix4<T> & M)
    {
        vector3<T> dst;
        mult(dst, M, *this);
        x = dst.x;
        y = dst.y;
        z = dst.z;
        return (*this);
    }
    // TL
    vector3<T> & operator/=(const vector3<T> & d)
    {
        x/= d.x;
        y/= d.y;
        z/= d.z;
        return *this;
    }
    vector3<T> & operator/=(const T & lambda)
    {
        x/= lambda;
        y/= lambda;
        z/= lambda;
        return *this;
    }

    vector3<T> & operator*=(const T & lambda)
    {
        x*= lambda;
        y*= lambda;
        z*= lambda;
        return *this;
    }
    // TL
    vector3<T> & operator*=(const vector3<T> & v)
    {
        x*= v.x;
        y*= v.y;
        z*= v.z;
        return *this;
    }

    vector3<T> operator - () const
	{
		return vector3<T>(-x, -y, -z);
	}

    vector3<T> & operator-=(const vector3<T> & u)
    {
        x-= u.x;
        y-= u.y;
        z-= u.z;
        return *this;
    }

    vector3<T> & operator+=(const vector3<T> & u)
    {
        x+= u.x;
        y+= u.y;
        z+= u.z;
        return *this;
    }
    vector3<T> & rotateBy(const quaternion<T>& q);
	T normalize();
	void orthogonalize( const vector3<T>& v );
	void orthonormalize( const vector3<T>& v )
	{
		orthogonalize( v ); //  just orthogonalize...
		normalize();        //  ...and normalize it
	}

	T sq_norm() const { return x * x + y * y + z * z; }
	T norm() const { return sqrtf(sq_norm()); }

    T & operator[](int i)
    {
        return vec_array[i];
    }

    const T operator[](int i) const
    {
        return vec_array[i];
    }

    union {
        struct {
            T x,y,z;        // standard names for components
        };
        struct {
            T s,t,r;        // standard names for components
        };
        T vec_array[3];     // array access
    };
};

template<class T>
inline const vector3<T> operator+(const vector3<T>& u, const vector3<T>& v)
{
	return vector3<T>(u.x + v.x, u.y + v.y, u.z + v.z);
}

template<class T>
inline const vector3<T> operator-(const vector3<T>& u, const vector3<T>& v)
{
    return vector3<T>(u.x - v.x, u.y - v.y, u.z - v.z);
}

template<class T>
inline const vector3<T> operator^(const vector3<T>& u, const vector3<T>& v)
{
    return vector3<T>(u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x);
}

template<class T>
inline const vector3<T> operator*(const T s, const vector3<T>& u)
{
	return vector3<T>(s * u.x, s * u.y, s * u.z);
}

template<class T>
inline const vector3<T> operator*(const vector3<T>& u, const T s)
{
	return vector3<T>(s * u.x, s * u.y, s * u.z);
}

template<class T>
inline const vector3<T> operator/(const vector3<T>& u, const T s)
{
	return vector3<T>(u.x / s, u.y / s, u.z / s);
}

template<class T>
inline const vector3<T> operator*(const vector3<T>& u, const vector3<T>& v)
{
	return vector3<T>(u.x * v.x, u.y * v.y, u.z * v.z);
}

template<class T>
inline vector2<T>::vector2(const vector3<T>& u)
{
	T k = 1 / u.z;
	x = k * u.x;
	y = k * u.y;
}

template<class T> extern vector4<T> & mult(vector4<T>& u, const matrix3<T>& M, const vector4<T>& v);
template<class T> extern vector4<T> & mult(vector4<T>& u, const matrix4<T>& M, const vector4<T>& v);

template<class T>
struct vector4
{
	vector4() { }
    vector4(T x) : x(x), y(x), z(x), w(x) { }
    vector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
    vector4(const T* xyzw) : x(xyzw[0]), y(xyzw[1]), z(xyzw[2]), w(xyzw[3]) { }
	vector4(const vector2<T>& u) : x(u.x), y(u.y), z(0.0f), w(1.0f) { }
	vector4(const vector2<T>& u, const T zz) : x(u.x), y(u.y), z(zz), w(1.0f) { }
	vector4(const vector2<T>& u, const T zz, const T ww) : x(u.x), y(u.y), z(zz), w(ww) { }
	vector4(const vector3<T>& u) : x(u.x), y(u.y), z(u.z), w(1.0f) { }
	vector4(const vector3<T>& u, const T w) : x(u.x), y(u.y), z(u.z), w(w) { }
	vector4(const vector4<T>& u) : x(u.x), y(u.y), z(u.z), w(u.w) { }

    bool operator==(const vector4<T> & u) const
    {
        return (u.x == x && u.y == y && u.z == z && u.w == w) ? true : false;
    }

    bool operator!=( const vector4<T>& rhs ) const
    {
        return !(*this == rhs );
    }

    // TL
    vector4<T> & operator*=(const matrix3<T> & M)
    {
        vector4<T> dst;
        mult(dst, M, *this);
        x = dst.x;
        y = dst.y;
        z = dst.z;
        w = dst.z;
        return (*this);
    }
    // TL
    vector4<T> & operator*=(const matrix4<T> & M)
    {
        vector4<T> dst;
        mult(dst, M, *this);
        x = dst.x;
        y = dst.y;
        z = dst.z;
        w = dst.w;
        return (*this);
    }
    // TL
    vector4<T> & operator/=(const T & lambda)
    {
        x/= lambda;
        y/= lambda;
        z/= lambda;
        w/= lambda;
        return *this;
    }

    vector4<T> & operator*=(const T & lambda)
    {
        x*= lambda;
        y*= lambda;
        z*= lambda;
        w*= lambda;
        return *this;
    }

    vector4<T> & operator-=(const vector4<T> & u)
    {
        x-= u.x;
        y-= u.y;
        z-= u.z;
        w-= u.w;
        return *this;
    }

    vector4<T> & operator+=(const vector4<T> & u)
    {
        x+= u.x;
        y+= u.y;
        z+= u.z;
        w+= u.w;
        return *this;
    }

    vector4<T> operator - () const
	{
		return vector4<T>(-x, -y, -z, -w);
	}

    T & operator[](int i)
    {
        return vec_array[i];
    }

    const T operator[](int i) const
    {
        return vec_array[i];
    }

    union {
        struct {
            T x,y,z,w;          // standard names for components
        };
        struct {
            T s,t,r,q;          // standard names for components
        };
        T vec_array[4];     // array access
    };
}; //struct vector4

template<class T>
inline const vector4<T> operator+(const vector4<T>& u, const vector4<T>& v)
{
	return vector4<T>(u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w);
}

template<class T>
inline const vector4<T> operator-(const vector4<T>& u, const vector4<T>& v)
{
    return vector4<T>(u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w);
}

template<class T>
inline const vector4<T> operator*(const T s, const vector4<T>& u)
{
	return vector4<T>(s * u.x, s * u.y, s * u.z, s * u.w);
}

template<class T>
inline const vector4<T> operator*(const vector4<T>& u, const T s)
{
	return vector4<T>(s * u.x, s * u.y, s * u.z, s * u.w);
}

template<class T>
inline const vector4<T> operator/(const vector4<T>& u, const T s)
{
    return vector4<T>(u.x / s, u.y / s, u.z / s, u.w / s);
}

template<class T>
inline const vector4<T> operator*(const vector4<T>& u, const vector4<T>& v)
{
	return vector4<T>(u.x * v.x, u.y * v.y, u.z * v.z, u.w * v.w);
}

template<class T>
inline vector3<T>::vector3(const vector4<T>& u)
{
	x = u.x;
	y = u.y;
	z = u.z;
}

// quaternion
template<class T> struct quaternion;

/*
    for all the matrices...a<x><y> indicates the element at row x, col y

    For example:
    a01 <-> row 0, col 1 
*/

template<class T>
struct matrix3
{
    matrix3();
    matrix3(const T * array);
    matrix3(const matrix3<T> & M);
    matrix3( const T& f0,  const T& f1,  const T& f2,  
          const T& f3,  const T& f4,  const T& f5,  
          const T& f6,  const T& f7,  const T& f8 )
  		  : a00( f0 ), a10( f1 ), a20( f2 ), 
            a01( f3 ), a11( f4 ), a21( f5 ),
  		    a02( f6 ), a12( f7 ), a22( f8) { }

    const vector3<T> col(const int i) const
    {
        return vector3<T>(&mat_array[i * 3]);
    }

    const vector3<T> operator[](int i) const
    {
        return vector3<T>(mat_array[i], mat_array[i + 3], mat_array[i + 6]);
    }

    const T& operator()(const int& i, const int& j) const
    {
        return mat_array[ j * 3 + i ];
    }

    T& operator()(const int& i, const int& j)
    {
        return  mat_array[ j * 3 + i ];
    }

    matrix3<T> & operator*=(const T & lambda)
    {
        for (int i = 0; i < 9; ++i)
            mat_array[i] *= lambda;
        return *this;
    }
    //TL
    matrix3<T> & operator*=(const matrix3<T> & M)
    {
        return *this;
    }

    matrix3<T> & operator-=(const matrix3<T> & M)
    {
        for (int i = 0; i < 9; ++i)
            mat_array[i] -= M.mat_array[i];
        return *this;
    }

    matrix3<T> & set_row(int i, const vector3<T> & v)
    {
        mat_array[i] = v.x;
        mat_array[i + 3] = v.y;
        mat_array[i + 6] = v.z;
        return *this;
    }

	matrix3<T> & set_col(int i, const vector3<T> & v)
	{
        mat_array[i * 3] = v.x;
        mat_array[i * 3 + 1] = v.y;
        mat_array[i * 3 + 2] = v.z;
        return *this;
	}

    matrix3<T> & set_rot(const T & theta, const vector3<T> & v);
    matrix3<T> & set_rot(const vector3<T> & u, const vector3<T> & v);

    // Matrix norms...
    // Compute || M ||
    //                1
    T norm_one();

    // Compute || M ||
    //                +inf
    T norm_inf();

    union {
        struct {
            T a00, a10, a20;        // standard names for components
            T a01, a11, a21;        // standard names for components
            T a02, a12, a22;        // standard names for components
        };
        T mat_array[9];     // array access
    };
};//struct matrix3

template<class T>
const vector3<T> operator*(const matrix3<T>&, const vector3<T>&);
template<class T>
const vector3<T> operator*(const vector3<T>&, const matrix3<T>&);

template<class T> extern matrix4<T> & mult(matrix4<T>& C, const matrix4<T>& A, const matrix4<T>& B);

//
// Note : as_...() means that the whole matrix is being modified
// set_...() only changes the concerned fields of the matrix
//
// translate()/scale()/identity()/rotate() : act as OpenGL functions. Example :
// M.identity()
// M.translate(t)
// M.scale(s)
// drawMyVertex(vtx)
//
// is like : newVtx = Midentiry * Mt * Ms * vtx
//
template<class T>
struct matrix4
{
    matrix4();
    matrix4(const T * array);
    matrix4(const matrix3<T> & M);
    matrix4(const matrix4<T> & M);

    matrix4( const T& f0,  const T& f1,  const T& f2,  const T& f3,
  		  const T& f4,  const T& f5,  const T& f6,  const T& f7,
  		  const T& f8,  const T& f9,  const T& f10, const T& f11,
  		  const T& f12, const T& f13, const T& f14, const T& f15 )
  		  : a00( f0 ), a10( f1 ), a20( f2 ), a30( f3 ),
  		    a01( f4 ), a11( f5 ), a21( f6 ), a31( f7 ),
  		    a02( f8 ), a12( f9 ), a22( f10), a32( f11),
			a03( f12), a13( f13), a23( f14), a33( f15) { }
 
    const vector4<T> col(const int i) const
    {
        return vector4<T>(&mat_array[i * 4]);
    }
    
    const vector4<T> operator[](const int& i) const
    {
        return vector4<T>(mat_array[i], mat_array[i + 4], mat_array[i + 8], mat_array[i + 12]);
    }
   
    const T& operator()(const int& i, const int& j) const
    {
        return mat_array[ j * 4 + i ];
    }

    T& operator()(const int& i, const int& j)
    {
        return  mat_array[ j * 4 + i ];
    }

    matrix4<T> & set_col(int i, const vector4<T> & v)
    {
        mat_array[i * 4] = v.x;
        mat_array[i * 4 + 1] = v.y;
        mat_array[i * 4 + 2] = v.z;
        mat_array[i * 4 + 3] = v.w;
        return *this;
    }

    matrix4<T> & set_row(int i, const vector4<T> & v)
    {
        mat_array[i] = v.x;
        mat_array[i + 4] = v.y;
        mat_array[i + 8] = v.z;
        mat_array[i + 12] = v.w;
        return *this;
    }

    matrix3<T> & get_rot(matrix3<T> & M) const;
    quaternion<T> & get_rot(quaternion<T> & q) const;
    matrix4<T> & set_rot(const quaternion<T> & q);
    matrix4<T> & set_rot(const matrix3<T> & M);
    matrix4<T> & set_rot(const T & theta, const vector3<T> & v);
    matrix4<T> & set_rot(const vector3<T> & u, const vector3<T> & v);

    matrix4<T> & as_rot(const quaternion<T> & q)
    {
        a30 = a31 = a32 = 0.0; a33 = 1.0;
        a03 = a13 = a23 = 0.0;
        set_rot(q);
        return *this;
    }
    matrix4<T> & as_rot(const matrix3<T> & M)
    {
        a30 = a31 = a32 = 0.0; a33 = 1.0;
        a03 = a13 = a23 = 0.0;
        set_rot(M);
        return *this;
    }
    matrix4<T> & as_rot(const T & theta, const vector3<T> & v)
    {
        set_rot(theta,v);
        a30 = a31 = a32 = 0.0; a33 = 1.0;
        a03 = a13 = a23 = 0.0;
        return *this;
    }
    matrix4<T> & as_rot(const vector3<T> & u, const vector3<T> & v)
    {
        a30 = a31 = a32 = 0.0; a33 = 1.0;
        a03 = a13 = a23 = 0.0;
        set_rot(u,v);
        return *this;
    }

	matrix4<T> & set_scale(const vector3<T>& s);
	vector3<T>& get_scale(vector3<T>& s) const;
    matrix4<T> & as_scale(const vector3<T>& s);
    matrix4<T> & as_scale(const T& s);
    matrix4<T> & set_translation(const vector3<T> & t);
    inline matrix4<T> & set_translate(const vector3<T> & t) { return set_translation(t); }
    vector3<T> & get_translation(vector3<T> & t) const;
    matrix4<T> & as_translation(const vector3<T> & t);

	matrix4<T> operator*(const matrix4<T>&) const;
    //TL
    matrix4<T> & operator*=(const matrix4<T> & M)
    {
        matrix4<T> Mtmp;
        *this = mult(Mtmp, *this, M);
        return *this;
    }

    //TL: some additional methods that look like OpenGL...
    // they behave the same as the OpenGL matrix system
    // But: using vector3<T> class; rotation is in Radians
    // TODO: optimize
    matrix4<T> & identity()
    {
        memcpy(mat_array, array16_id, sizeof(T) * 16);
        return *this;
    }
    matrix4<T> & translate(vector3<T> t)
    {
        *this *= matrix4<T>().as_translation(t);
        return *this;
    }
    matrix4<T> & translate(T *t)
    {
        *this *= matrix4<T>().as_translation(*(vector3<T>*)t);
        return *this;
    }
    matrix4<T> & scale(vector3<T> s)
    {
        *this *= matrix4<T>().as_scale(s);
        return *this;
    }
    matrix4<T> & scale(T s)
    {
        *this *= matrix4<T>().as_scale(s);
        return *this;
    }
    matrix4<T> & rotate(const T & theta, const vector3<T> & v)
    {
        *this *= matrix4<T>().as_rot(theta, v);
        return *this;
    }
    matrix4<T> & rotate(quaternion<T> &q)
    {
        *this *= matrix4<T>().identity().set_rot(q);
        return *this;
    }

    union {
        struct {
            T a00, a10, a20, a30;   // standard names for components
            T a01, a11, a21, a31;   // standard names for components
            T a02, a12, a22, a32;   // standard names for components
            T a03, a13, a23, a33;   // standard names for components
        };
        struct {
            T _11, _12, _13, _14;   // standard names for components
            T _21, _22, _23, _24;   // standard names for components
            T _31, _32, _33, _34;   // standard names for components
            T _41, _42, _43, _44;   // standard names for components
        };
        union {
            struct {
                T b00, b10, b20, p; // standard names for components
                T b01, b11, b21, q; // standard names for components
                T b02, b12, b22, r; // standard names for components
                T x, y, z, w;       // standard names for components
            };
        };
        T mat_array[16];     // array access
    };
};//struct matrix4

template<class T>
const vector4<T> operator*(const matrix4<T>&, const vector4<T>&);
template<class T>
const vector4<T> operator*(const matrix4<T>&, const vector3<T>&);
template<class T>
const vector4<T> operator*(const vector4<T>&, const matrix4<T>&);

template<class T>
matrix4<T> & translation(matrix4<T> & m, vector3<T> t)
{
    m.as_translation(t);
    return m;
}
template<class T>
matrix4<T> & translation(matrix4<T> & m, T x, T y, T z)
{
    m.as_translation(vector3<T>(x,y,z));
    return m;
}

// quaternion<T>ernion
template<class T>
struct quaternion
{
public:
    quaternion();
	quaternion(T *q);
	quaternion(T x, T y, T z, T w);
	quaternion(const quaternion<T>& quaternion);
	quaternion(const vector3<T>& axis, T angle);
	quaternion(const vector3<T>& eulerXYZ); // From Euler
	quaternion(const matrix3<T>& rot);
	quaternion(const matrix4<T>& rot);
	quaternion<T>& operator=(const quaternion<T>& quaternion);
	quaternion<T> operator-()
	{
		return quaternion<T>(-x, -y, -z, -w);
	}
    quaternion<T> inverse();
    quaternion<T> conjugate();
	void normalize();
	void from_matrix(const matrix3<T>& mat);
	void from_matrix(const matrix4<T>& mat);
	void to_matrix(matrix3<T>& mat) const;
	void to_matrix(matrix4<T>& mat) const;
    void to_euler_xyz(vector3<T> & r);
    void to_euler_xyz(T* r);
    void from_euler_xyz(vector3<T> r);
	quaternion<T>& operator*=(const quaternion<T>& q);
	static const quaternion<T> Identity;
	T& operator[](int i) { return comp[i]; }
	const T operator[](int i) const { return comp[i]; }
	union {
		struct {
			T x, y, z, w;
		};
		T comp[4];
	};
};//struct quaternion
template<class T> const quaternion<T> operator*(const quaternion<T>&, const quaternion<T>&);
template<class T> extern quaternion<T> & add_quats(quaternion<T> & p, const quaternion<T> & q1, const quaternion<T> & q2);
template<class T>
extern T dot(const quaternion<T> & p, const quaternion<T> & q);
template<class T>
extern quaternion<T> & dot(T s, const quaternion<T> & p, const quaternion<T> & q);
template<class T>
extern quaternion<T> & slerp_quats(quaternion<T> & p, T s, const quaternion<T> & q1, const quaternion<T> & q2);
template<class T>
extern quaternion<T> & axis_to_quat(quaternion<T> & q, const vector3<T> & a, const T phi);
template<class T>
extern matrix3<T> & quat_2_mat(matrix3<T> &M, const quaternion<T> &q );
template<class T>
extern quaternion<T> & mat_2_quat(quaternion<T> &q,const matrix3<T> &M);

// constant algebraic values

static const vector2<nv_scalar>      vec2f_zero(nv_zero,nv_zero);
static const vector4<nv_scalar>      vec4f_one(nv_one,nv_one,nv_one,nv_one);
static const vector3<nv_scalar>      vec3f_one(nv_one,nv_one,nv_one);
static const vector3<nv_scalar>      vec3f_zero(nv_zero,nv_zero,nv_zero);
static const vector3<nv_scalar>      vec3f_x(nv_one,nv_zero,nv_zero);
static const vector3<nv_scalar>      vec3f_y(nv_zero,nv_one,nv_zero);
static const vector3<nv_scalar>      vec3f_z(nv_zero,nv_zero,nv_one);
static const vector3<nv_scalar>      vec3f_neg_x(-nv_one,nv_zero,nv_zero);
static const vector3<nv_scalar>      vec3f_neg_y(nv_zero,-nv_one,nv_zero);
static const vector3<nv_scalar>      vec3f_neg_z(nv_zero,nv_zero,-nv_one);
static const vector4<nv_scalar>      vec4f_zero(nv_zero,nv_zero,nv_zero,nv_zero);
static const vector4<nv_scalar>      vec4f_x(nv_one,nv_zero,nv_zero,nv_zero);
static const vector4<nv_scalar>      vec4f_neg_x(-nv_one,nv_zero,nv_zero,nv_zero);
static const vector4<nv_scalar>      vec4f_y(nv_zero,nv_one,nv_zero,nv_zero);
static const vector4<nv_scalar>      vec4f_neg_y(nv_zero,-nv_one,nv_zero,nv_zero);
static const vector4<nv_scalar>      vec4f_z(nv_zero,nv_zero,nv_one,nv_zero);
static const vector4<nv_scalar>      vec4f_neg_z(nv_zero,nv_zero,-nv_one,nv_zero);
static const vector4<nv_scalar>      vec4f_w(nv_zero,nv_zero,nv_zero,nv_one);
static const vector4<nv_scalar>      vec4f_neg_w(nv_zero,nv_zero,nv_zero,-nv_one);
static const quaternion<nv_scalar>         quat_id(nv_zero,nv_zero,nv_zero,nv_one);
static const matrix4<nv_scalar>      mat4f_id(array16_id);
static const matrix3<nv_scalar>         mat3f_id(array9_id);
static const matrix4<nv_scalar>      mat4f_zero(array16_zero);
static const matrix4<nv_scalar>      mat4f_scale_bias(array16_scale_bias);

// normalizes a vector and return a reference of itself
template<class T> extern vector2<T> & normalize(vector2<T> & u);
template<class T> extern vector3<T> & normalize(vector3<T> & u);
template<class T> extern vector4<T> & normalize(vector4<T> & u);

// Computes the squared magnitude
template<class T>
inline T nv_sq_norm(const vector3<T> & n)
{ return n.x * n.x + n.y * n.y + n.z * n.z; }

template<class T> 
inline T nv_sq_norm(const vector4<T> & n)
{ return n.x * n.x + n.y * n.y + n.z * n.z + n.w * n.w; }

// Computes the magnitude
template<class T> 
inline T nv_norm(const vector3<T> & n)
{ return sqrtf(nv_sq_norm(n)); }

template<class T> 
inline T nv_norm(const vector4<T> & n)
{ return sqrtf(nv_sq_norm(n)); }


// computes the cross product ( v cross w) and stores the result in u
// i.e.     u = v cross w
template<class T> extern vector3<T> & cross(vector3<T> & u, const vector3<T> & v, const vector3<T> & w);

// computes the dot product ( v dot w) and stores the result in u
// i.e.     u = v dot w
template<class T> extern T & dot(T & u, const vector3<T> & v, const vector3<T> & w);
template<class T> extern T dot(const vector3<T> & v, const vector3<T> & w);
template<class T> extern T & dot(T & u, const vector4<T> & v, const vector4<T> & w);
template<class T> extern T dot(const vector4<T> & v, const vector4<T> & w);
template<class T> extern T & dot(T & u, const vector3<T> & v, const vector4<T> & w);
template<class T> extern T dot(const vector3<T> & v, const vector4<T> & w);
template<class T> extern T & dot(T & u, const vector4<T> & v, const vector3<T> & w);
template<class T> extern T dot(const vector4<T> & v, const vector3<T> & w);
template<class T> extern T & dot(T & u, const vector2<T> & v, const vector2<T> & w);
template<class T> extern T dot(const vector2<T> & v, const vector2<T> & w);

// compute the reflected vector R of L w.r.t N - vectors need to be 
// normalized
//
//                R     N     L
//                  _       _
//                 |\   ^   /|
//                   \  |  /
//                    \ | /
//                     \|/
//                      +
template<class T> extern vector3<T> & reflect(vector3<T> & r, const vector3<T> & n, const vector3<T> & l);

// Computes u = v * lambda + u
template<class T> extern vector3<T> & madd(vector3<T> & u, const vector3<T> & v, const T & lambda);
// Computes u = v * lambda
template<class T> extern vector3<T> & mult(vector3<T> & u, const vector3<T> & v, const T & lambda);
// Computes u = v * w
template<class T> extern vector3<T> & mult(vector3<T> & u, const vector3<T> & v, const vector3<T> & w);
// Computes u = v + w
template<class T> extern vector3<T> & add(vector3<T> & u, const vector3<T> & v, const vector3<T> & w);
// Computes u = v - w
template<class T> extern vector3<T> & sub(vector3<T> & u, const vector3<T> & v, const vector3<T> & w);

//template<class T> extern vector2<T> & sub(vector2<T> & u, const vector2<T> & v, const vector2<T> & w);
//template<class T> extern vector2<T> sub(const vector2<T> & v, const vector2<T> & w);

// Computes u = u * s
template<class T> extern vector2<T> & scale(vector2<T> & u, const T s);
template<class T> extern vector3<T> & scale(vector3<T> & u, const T s);
template<class T> extern vector4<T> & scale(vector4<T> & u, const T s);

template<class T> inline matrix4<T> & scale(matrix4<T> & u, const vector3<T> s)
{
    u.as_scale(s);
    return u;
}

// Computes u = M * v
template<class T> extern vector3<T> & mult(vector3<T> & u, const matrix3<T> & M, const vector3<T> & v);
template<class T> extern vector4<T> & mult(vector4<T> & u, const matrix4<T> & M, const vector4<T> & v);

// Computes u = v * M
template<class T> extern vector3<T> & mult(vector3<T> & u, const vector3<T> & v, const matrix3<T> & M);
template<class T> extern vector4<T> & mult(vector4<T> & u, const vector4<T> & v, const matrix4<T> & M);

// Computes u = M(4x4) * v and divides by w
template<class T> extern vector3<T> & mult_pos(vector3<T> & u, const matrix4<T> & M, const vector3<T> & v);
// Computes u = M(4x4) * v
template<class T> extern vector3<T> & mult_dir(vector3<T> & u, const matrix4<T> & M, const vector3<T> & v);
// Computes u = M(4x4) * v and does not divide by w (assumed to be 1)
template<class T> extern vector3<T> & mult(vector3<T>& u, const matrix4<T>& M, const vector3<T>& v);

// Computes u = v * M(4x4) and divides by w
template<class T> extern vector3<T> & mult_pos(vector3<T> & u, const vector3<T> & v, const matrix4<T> & M);
// Computes u = v * M(4x4)
template<class T> extern vector3<T> & mult_dir(vector3<T> & u, const vector3<T> & v, const matrix4<T> & M);
// Computes u = v * M(4x4) and does not divide by w (assumed to be 1)
template<class T> extern vector3<T> & mult(vector3<T>& u, const vector3<T>& v, const matrix4<T>& M);

// Computes A += B
template<class T> extern matrix4<T> & add(matrix4<T> & A, const matrix4<T> & B);
template<class T> extern matrix3<T> & add(matrix3<T> & A, const matrix3<T> & B);

// Computes C = A + B
template<class T> extern matrix4<T> & add(matrix4<T> & C, const matrix4<T> & A, const matrix4<T> & B);
template<class T> extern matrix3<T> & add(matrix3<T> & C, const matrix3<T> & A, const matrix3<T> & B);

// Computes C = A * B
template<class T> extern matrix4<T> & mult(matrix4<T> & C, const matrix4<T> & A, const matrix4<T> & B);
template<class T> extern matrix3<T> & mult(matrix3<T> & C, const matrix3<T> & A, const matrix3<T> & B);

// Compute M = -M
template<class T> extern matrix4<T> & negate(matrix4<T> & M);
template<class T> extern matrix3<T> & negate(matrix3<T> & M);

// Computes B = Transpose(A)
//       T
//  B = A
template<class T> extern matrix3<T> & transpose(matrix3<T> & B, const matrix3<T> & A);
template<class T> extern matrix4<T> & transpose(matrix4<T> & B, const matrix4<T> & A);

// Computes B = Transpose(B)
//       T
//  B = B
template<class T> extern matrix3<T> & transpose(matrix3<T> & B);
template<class T> extern matrix4<T> & transpose(matrix4<T> & B);

// Computes B = inverse(A)
//       -1
//  B = A
template<class T> extern matrix4<T> & invert(matrix4<T> & B, const matrix4<T> & A);
template<class T> extern matrix3<T> & invert(matrix3<T> & B, const matrix3<T> & A);
template<class T> extern matrix4<T>   inverse(const matrix4<T> & A)
{
    matrix4<T> M;
    return invert(M, A);
}

// Computes B = inverse(A)
//                                       T  T
//                   (R t)             (R -R t)
// assuming that A = (0 1) so that B = (0    1)
//  B = A
template<class T> extern matrix4<T> & invert_rot_trans(matrix4<T> & B, const matrix4<T> & A);

template<class T> extern matrix4<T> & look_at(matrix4<T> & M, const vector3<T> & eye, const vector3<T> & center, const vector3<T> & up);
template<class T> extern matrix4<T> & frustum(matrix4<T> & M, const T l, const T r, const T b, 
               const T t, const T n, const T f);

template<class T> extern matrix4<T> & perspective(matrix4<T> & M, const T fovy, const T aspect, const T n, const T f);
template<class T> extern matrix4<T> & ortho(matrix4<T> & M, const T left, 
                              const T right, 
                              const T bottom, 
                              const T top,
                              const T n,
                              const T f);

/* Decompose Affine Matrix 
 *    A = TQS, where
 * A is the affine transform
 * T is the translation vector
 * Q is the rotation (quaternion)
 * S is the scale vector
 * f is the sign of the determinant
*/
template<class T> extern void decomp_affine(const matrix4<T> & A, vector3<T> & v3, quaternion<T> & Q, quaternion<T> & U, vector3<T> & S, T & f);
// quaternion<T>ernion
template<class T> extern quaternion<T> & normalize(quaternion<T> & p);
template<class T> extern quaternion<T> & conj(quaternion<T> & p);
template<class T> extern quaternion<T> & conj(quaternion<T> & p, const quaternion<T> & q);
template<class T> extern quaternion<T> & add_quats(quaternion<T> & p, const quaternion<T> & q1, const quaternion<T> & q2);
template<class T> extern quaternion<T> & axis_to_quat(quaternion<T> & q, const vector3<T> & a, const T phi);
template<class T> extern matrix3<T> & quat_2_mat(matrix3<T> &M, const quaternion<T> &q );
template<class T> extern quaternion<T> & mat_2_quat(quaternion<T> &q,const matrix3<T> &M);
template<class T> extern quaternion<T> & mat_2_quat(quaternion<T> &q,const matrix4<T> &M);

// surface properties
template<class T> extern matrix3<T> & tangent_basis(matrix3<T> & basis,const vector3<T> & v0,const vector3<T> & v1,const vector3<T> & v2,const vector2<T> & t0,const vector2<T> & t1,const vector2<T> & t2, const vector3<T> & n);

// linear interpolation
#ifdef USEOPTIX
#pragma message("**WARNING** nv_math.h : Canceling the lerp() function here : already declared in OptiX")
#else
template<class T>
inline T lerp(T t, T a, T b)
{ return a * (nv_one - t) + t * b; }

template<class T>
inline vector3<T> & lerp(vector3<T> & w, const T & t, const vector3<T> & u, const vector3<T> & v)
{ w.x = lerp(t, u.x, v.x); w.y = lerp(t, u.y, v.y); w.z = lerp(t, u.z, v.z); return w; }

template<class T>
inline vector4<T> & lerp(vector4<T> & w, const T & t, const vector4<T> & u, const vector4<T> & v)
{ w.x = lerp(t, u.x, v.x); w.y = lerp(t, u.y, v.y); w.z = lerp(t, u.z, v.z); w.w = lerp(t, u.w, v.w); return w; }
#endif

// utilities
template<class T> 
inline T nv_min(const T & lambda, const T & n)
{ return (lambda < n ) ? lambda : n; }

template<class T> 
inline T nv_max(const T & lambda, const T & n)
{ return (lambda > n ) ? lambda : n; }

template<class T> 
inline T nv_clamp(T u, const T min, const T max)
{ u = (u < min) ? min : u; u = (u > max) ? max : u; return u; }

template<class T> extern T nv_random();

template<class T> extern quaternion<T> & trackball(quaternion<T> & q, vector2<T> & pt1, vector2<T> & pt2, T trackballsize);

template<class T> extern vector3<T> & cube_map_normal(int i, int x, int y, int cubesize, vector3<T> & v);

// Componentwise maximum and minium 
template<class T> 
inline void nv_max(vector3<T> & vOut, const vector3<T> & vFirst, const vector3<T> & vSecond)
{
    vOut.x = nv_max(vFirst.x, vSecond.x);
    vOut.y = nv_max(vFirst.y, vSecond.y);
    vOut.z = nv_max(vFirst.z, vSecond.z);
}

template<class T> 
inline void nv_min(vector3<T> & vOut, const vector3<T> & vFirst, const vector3<T> & vSecond)
{
    vOut.x = nv_min(vFirst.x, vSecond.x);
    vOut.y = nv_min(vFirst.y, vSecond.y);
    vOut.z = nv_min(vFirst.z, vSecond.z);
}


// geometry
// computes the area of a triangle
template<class T> extern T nv_area(const vector3<T> & v1, const vector3<T> & v2, const vector3<T> &v3);
// computes the perimeter of a triangle
template<class T> extern T nv_perimeter(const vector3<T> & v1, const vector3<T> & v2, const vector3<T> &v3);
// find the inscribed circle
template<class T> extern T nv_find_in_circle( vector3<T> & center, const vector3<T> & v1, const vector3<T> & v2, const vector3<T> &v3);
// find the circumscribed circle
template<class T> extern T nv_find_circ_circle( vector3<T> & center, const vector3<T> & v1, const vector3<T> & v2, const vector3<T> &v3);

// fast cosine functions
template<class T> extern T fast_cos(const T x);
template<class T> extern T ffast_cos(const T x);

// determinant
template<class T> T det(const matrix3<T> & A);

template<class T> extern void nv_is_valid(const vector3<T>& v);
template<class T> extern void nv_is_valid(T lambda);

// TL : v1 and v2 MUST be normalized. Not done inot this to avoid redundant work...
template<class T> extern T getAngle(const vector3<T> & v1, const vector3<T> & v2);
template<class T> extern vector3<T> & rotateBy(vector3<T> & dst, const vector3<T> & src, const quaternion<T>& q);

template<class T>
matrix4<T>& rotationYawPitchRoll( matrix4<T> & M, const T yaw , const T pitch , const T roll )
{
    memcpy(M.mat_array, array16_id, sizeof(T) * 16);
    matrix4<T> rot;

    if (roll)
    {
        M.rotate(roll, vector3<T>(0,0,1));
    }
    if (pitch)
    {
        M.rotate(pitch, vector3<T>(1,0,0));
    }
    if (yaw)
    {
        M.rotate(yaw, vector3<T>(0,1,0));
    }

    return M;
}



typedef vector2<nv_scalar> vec2f;
typedef vector3<nv_scalar> vec3f;
typedef vector4<nv_scalar> vec4f;
typedef matrix3<nv_scalar> mat3f;
typedef matrix4<nv_scalar> mat4f;
typedef matrix4<nv_scalar> matrix4f;
typedef quaternion<nv_scalar> quatf;

typedef vector2<int> vec2i;
typedef vector3<int> vec3i;
typedef vector4<int> vec4i;

typedef vector2<unsigned int> vec2ui;
typedef vector3<unsigned int> vec3ui;
typedef vector4<unsigned int> vec4ui;

typedef unsigned int uint;

template nv_scalar nv_sq_norm(const vector3<nv_scalar> & n);
template nv_scalar nv_sq_norm(const vector4<nv_scalar> & n);
template nv_scalar nv_norm(const vector3<nv_scalar> & n);
template nv_scalar nv_norm(const vector4<nv_scalar> & n);

template vector3<nv_scalar> & reflect(vector3<nv_scalar> & r, const vector3<nv_scalar> & n, const vector3<nv_scalar> & l);
template vector3<nv_scalar> & madd(vector3<nv_scalar> & u, const vector3<nv_scalar> & v, const nv_scalar & lambda);
template vector3<nv_scalar> & mult(vector3<nv_scalar> & u, const vector3<nv_scalar> & v, const nv_scalar & lambda);
template vector3<nv_scalar> & mult(vector3<nv_scalar> & u, const vector3<nv_scalar> & v, const vector3<nv_scalar> & w);
template vector3<nv_scalar> & add(vector3<nv_scalar> & u, const vector3<nv_scalar> & v, const vector3<nv_scalar> & w);
template vector3<nv_scalar> & sub(vector3<nv_scalar> & u, const vector3<nv_scalar> & v, const vector3<nv_scalar> & w);
template vector2<nv_scalar> & scale(vector2<nv_scalar> & u, const nv_scalar s);
template vector3<nv_scalar> & scale(vector3<nv_scalar> & u, const nv_scalar s);
template vector4<nv_scalar> & scale(vector4<nv_scalar> & u, const nv_scalar s);
template<class nv_scalar> inline matrix4<nv_scalar> & scale(matrix4<nv_scalar> & u, const vector3<nv_scalar> s);
template vector3<nv_scalar> & mult(vector3<nv_scalar> & u, const matrix3<nv_scalar> & M, const vector3<nv_scalar> & v);
template vector4<nv_scalar> & mult(vector4<nv_scalar> & u, const matrix4<nv_scalar> & M, const vector4<nv_scalar> & v);
template vector3<nv_scalar> & mult(vector3<nv_scalar> & u, const vector3<nv_scalar> & v, const matrix3<nv_scalar> & M);
template vector4<nv_scalar> & mult(vector4<nv_scalar> & u, const vector4<nv_scalar> & v, const matrix4<nv_scalar> & M);
template vector3<nv_scalar> & mult_pos(vector3<nv_scalar> & u, const matrix4<nv_scalar> & M, const vector3<nv_scalar> & v);
template vector3<nv_scalar> & mult_dir(vector3<nv_scalar> & u, const matrix4<nv_scalar> & M, const vector3<nv_scalar> & v);
template vector3<nv_scalar> & mult(vector3<nv_scalar>& u, const matrix4<nv_scalar>& M, const vector3<nv_scalar>& v);
template vector3<nv_scalar> & mult_pos(vector3<nv_scalar> & u, const vector3<nv_scalar> & v, const matrix4<nv_scalar> & M);
template vector3<nv_scalar> & mult_dir(vector3<nv_scalar> & u, const vector3<nv_scalar> & v, const matrix4<nv_scalar> & M);
template vector3<nv_scalar> & mult(vector3<nv_scalar>& u, const vector3<nv_scalar>& v, const matrix4<nv_scalar>& M);
template matrix4<nv_scalar> & add(matrix4<nv_scalar> & A, const matrix4<nv_scalar> & B);
template matrix3<nv_scalar> & add(matrix3<nv_scalar> & A, const matrix3<nv_scalar> & B);
template matrix4<nv_scalar> & add(matrix4<nv_scalar> & C, const matrix4<nv_scalar> & A, const matrix4<nv_scalar> & B);
template matrix3<nv_scalar> & add(matrix3<nv_scalar> & C, const matrix3<nv_scalar> & A, const matrix3<nv_scalar> & B);
template matrix4<nv_scalar> & mult(matrix4<nv_scalar> & C, const matrix4<nv_scalar> & A, const matrix4<nv_scalar> & B);
template matrix3<nv_scalar> & mult(matrix3<nv_scalar> & C, const matrix3<nv_scalar> & A, const matrix3<nv_scalar> & B);
template matrix4<nv_scalar> & negate(matrix4<nv_scalar> & M);
template matrix3<nv_scalar> & negate(matrix3<nv_scalar> & M);
template matrix3<nv_scalar> & transpose(matrix3<nv_scalar> & B, const matrix3<nv_scalar> & A);
template matrix4<nv_scalar> & transpose(matrix4<nv_scalar> & B, const matrix4<nv_scalar> & A);
template matrix3<nv_scalar> & transpose(matrix3<nv_scalar> & B);
template matrix4<nv_scalar> & transpose(matrix4<nv_scalar> & B);
template matrix4<nv_scalar> & invert(matrix4<nv_scalar> & B, const matrix4<nv_scalar> & A);
template matrix3<nv_scalar> & invert(matrix3<nv_scalar> & B, const matrix3<nv_scalar> & A);
template matrix4<nv_scalar> & invert_rot_trans(matrix4<nv_scalar> & B, const matrix4<nv_scalar> & A);
template matrix4<nv_scalar> & look_at(matrix4<nv_scalar> & M, const vector3<nv_scalar> & eye, const vector3<nv_scalar> & center, const vector3<nv_scalar> & up);
template matrix4<nv_scalar> & frustum(matrix4<nv_scalar> & M, const nv_scalar l, const nv_scalar r, const nv_scalar b, const nv_scalar t, const nv_scalar n, const nv_scalar f);
template matrix4<nv_scalar> & perspective(matrix4<nv_scalar> & M, const nv_scalar fovy, const nv_scalar aspect, const nv_scalar n, const nv_scalar f);
template matrix4<nv_scalar> & ortho(matrix4<nv_scalar> & M, const nv_scalar left, 
                              const nv_scalar right, 
                              const nv_scalar bottom, 
                              const nv_scalar top,
                              const nv_scalar n,
                              const nv_scalar f);
template void decomp_affine(const matrix4<nv_scalar> & A, vector3<nv_scalar> & v3, quaternion<nv_scalar> & Q, quaternion<nv_scalar> & U, vector3<nv_scalar> & S, nv_scalar & f);
template quaternion<nv_scalar> & normalize(quaternion<nv_scalar> & p);
template quaternion<nv_scalar> & conj(quaternion<nv_scalar> & p);
template quaternion<nv_scalar> & conj(quaternion<nv_scalar> & p, const quaternion<nv_scalar> & q);
template quaternion<nv_scalar> & add_quats(quaternion<nv_scalar> & p, const quaternion<nv_scalar> & q1, const quaternion<nv_scalar> & q2);
template quaternion<nv_scalar> & axis_to_quat(quaternion<nv_scalar> & q, const vector3<nv_scalar> & a, const nv_scalar phi);
template matrix3<nv_scalar> & quat_2_mat(matrix3<nv_scalar> &M, const quaternion<nv_scalar> &q );
template quaternion<nv_scalar> & mat_2_quat(quaternion<nv_scalar> &q,const matrix3<nv_scalar> &M);
template quaternion<nv_scalar> & mat_2_quat(quaternion<nv_scalar> &q,const matrix4<nv_scalar> &M);
template matrix3<nv_scalar> & tangent_basis(matrix3<nv_scalar> & basis,const vector3<nv_scalar> & v0,const vector3<nv_scalar> & v1,const vector3<nv_scalar> & v2,const vector2<nv_scalar> & t0,const vector2<nv_scalar> & t1,const vector2<nv_scalar> & t2, const vector3<nv_scalar> & n);
template nv_scalar lerp(nv_scalar t, nv_scalar a, nv_scalar b);
template vector3<nv_scalar> & lerp(vector3<nv_scalar> & w, const nv_scalar & t, const vector3<nv_scalar> & u, const vector3<nv_scalar> & v);
template vector4<nv_scalar> & lerp(vector4<nv_scalar> & w, const nv_scalar & t, const vector4<nv_scalar> & u, const vector4<nv_scalar> & v);
template nv_scalar nv_min(const nv_scalar & lambda, const nv_scalar & n);
template nv_scalar nv_max(const nv_scalar & lambda, const nv_scalar & n);
template nv_scalar nv_clamp(nv_scalar u, const nv_scalar min, const nv_scalar max);

template void nv_max(vector3<nv_scalar> & vOut, const vector3<nv_scalar> & vFirst, const vector3<nv_scalar> & vSecond);
template void nv_min(vector3<nv_scalar> & vOut, const vector3<nv_scalar> & vFirst, const vector3<nv_scalar> & vSecond);
template const vector2<nv_scalar> operator+(const vector2<nv_scalar>& u, const vector2<nv_scalar>& v);
template const vector2<nv_scalar> operator-(const vector2<nv_scalar>& u, const vector2<nv_scalar>& v);
template const vector2<nv_scalar> operator+(const vector2<nv_scalar>& u, const nv_scalar s);
template const vector2<nv_scalar> operator-(const vector2<nv_scalar>& u, const nv_scalar s);
template const vector2<nv_scalar> operator*(const nv_scalar s, const vector2<nv_scalar>& u);
template const vector2<nv_scalar> operator*(const vector2<nv_scalar>& u, const nv_scalar s);
template const vector2<nv_scalar> operator/(const vector2<nv_scalar>& u, const nv_scalar s);
template const vector2<nv_scalar> operator*(const vector2<nv_scalar>&u, const vector2<nv_scalar>&v);

template const vector3<nv_scalar> operator+(const vector3<nv_scalar>& u, const vector3<nv_scalar>& v);
template const vector3<nv_scalar> operator-(const vector3<nv_scalar>& u, const vector3<nv_scalar>& v);
template const vector3<nv_scalar> operator^(const vector3<nv_scalar>& u, const vector3<nv_scalar>& v);
template const vector3<nv_scalar> operator*(const nv_scalar s, const vector3<nv_scalar>& u);
template const vector3<nv_scalar> operator*(const vector3<nv_scalar>& u, const nv_scalar s);
template const vector3<nv_scalar> operator/(const vector3<nv_scalar>& u, const nv_scalar s);
template const vector3<nv_scalar> operator*(const vector3<nv_scalar>& u, const vector3<nv_scalar>& v);
template vector2<nv_scalar>::vector2(const vector3<nv_scalar>& u);

template const vector4<nv_scalar> operator+(const vector4<nv_scalar>& u, const vector4<nv_scalar>& v);
template const vector4<nv_scalar> operator-(const vector4<nv_scalar>& u, const vector4<nv_scalar>& v);
template const vector4<nv_scalar> operator*(const nv_scalar s, const vector4<nv_scalar>& u);
template const vector4<nv_scalar> operator*(const vector4<nv_scalar>& u, const nv_scalar s);
template const vector4<nv_scalar> operator/(const vector4<nv_scalar>& u, const nv_scalar s);
template const vector4<nv_scalar> operator*(const vector4<nv_scalar>& u, const vector4<nv_scalar>& v);
template vector3<nv_scalar>::vector3(const vector4<nv_scalar>& u);

template const vector4<nv_scalar> operator*(const matrix4<nv_scalar>&, const vector4<nv_scalar>&);
template const vector4<nv_scalar> operator*(const matrix4<nv_scalar>&, const vector3<nv_scalar>&);
template const vector4<nv_scalar> operator*(const vector4<nv_scalar>&, const matrix4<nv_scalar>&);
template const vector3<nv_scalar> operator*(const matrix3<nv_scalar>&, const vector3<nv_scalar>&);
template const vector3<nv_scalar> operator*(const vector3<nv_scalar>&, const matrix3<nv_scalar>&);


template nv_scalar dot(const quaternion<nv_scalar> & p, const quaternion<nv_scalar> & q);
template quaternion<nv_scalar> & dot(nv_scalar s, const quaternion<nv_scalar> & p, const quaternion<nv_scalar> & q);
template quaternion<nv_scalar> & slerp_quats(quaternion<nv_scalar> & p, nv_scalar s, const quaternion<nv_scalar> & q1, const quaternion<nv_scalar> & q2);
template quaternion<nv_scalar> & axis_to_quat(quaternion<nv_scalar> & q, const vector3<nv_scalar> & a, const nv_scalar phi);
template matrix3<nv_scalar> & quat_2_mat(matrix3<nv_scalar> &M, const quaternion<nv_scalar> &q );
template quaternion<nv_scalar> & mat_2_quat(quaternion<nv_scalar> &q,const matrix3<nv_scalar> &M);
template nv_scalar & dot(nv_scalar & u, const vector3<nv_scalar> & v, const vector3<nv_scalar> & w);
template nv_scalar dot(const vector3<nv_scalar> & v, const vector3<nv_scalar> & w);
template nv_scalar & dot(nv_scalar & u, const vector4<nv_scalar> & v, const vector4<nv_scalar> & w);
template nv_scalar dot(const vector4<nv_scalar> & v, const vector4<nv_scalar> & w);
template nv_scalar & dot(nv_scalar & u, const vector3<nv_scalar> & v, const vector4<nv_scalar> & w);
template nv_scalar dot(const vector3<nv_scalar> & v, const vector4<nv_scalar> & w);
template nv_scalar & dot(nv_scalar & u, const vector4<nv_scalar> & v, const vector3<nv_scalar> & w);
template nv_scalar dot(const vector4<nv_scalar> & v, const vector3<nv_scalar> & w);
template nv_scalar dot(const vector2<nv_scalar> & v, const vector2<nv_scalar> & w);

template matrix4<nv_scalar> & translation(matrix4<nv_scalar> & m, vector3<nv_scalar> t);
template matrix4<nv_scalar> & translation(matrix4<nv_scalar> & m, nv_scalar x, nv_scalar y, nv_scalar z);

template matrix4<nv_scalar>& rotationYawPitchRoll( matrix4<nv_scalar> & M, const nv_scalar yaw , const nv_scalar pitch , const nv_scalar roll );

//template vector2<nv_scalar> & sub(vector2<nv_scalar> & u, const vector2<nv_scalar> & v, const vector2<nv_scalar> & w);
//template vector2<nv_scalar> sub(const vector2<nv_scalar> & v, const vector2<nv_scalar> & w);
}//namespace nv_math

#endif //_nv_math_h_
