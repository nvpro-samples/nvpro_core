/*********************************************************************NVMH1****
File:
nv_algebra.cpp

Copyright (C) 1999, 2000 NVIDIA Corporation
This file is provided without support, instruction, or implied warranty of any
kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
not liable under any circumstances for any damages or loss whatsoever arising
from the use or inability to use this file or items derived from it.

Comments: 


******************************************************************************/

#ifndef _nv_math_h_
#include "nv_math.h"
#endif
#ifndef _WIN32
#define _isnan isnan
#define _finite finite
#endif

namespace nv_math
{

template<class T>
matrix3<T>::matrix3()
{
}

template<class T>
matrix3<T>::matrix3(const T* array)
{
    memcpy(mat_array, array, sizeof(T) * 9);
}

template<class T>
matrix3<T>::matrix3(const matrix3<T> & M)
{
    memcpy(mat_array, M.mat_array, sizeof(T) * 9);
}

template<class T>
matrix4<T>::matrix4()
{
}

template<class T>
matrix4<T>::matrix4(const T* array)
{
    memcpy(mat_array, array, sizeof(T) * 16);
}

template<class T>
matrix4<T>::matrix4(const matrix4<T>& M)
{
    memcpy(mat_array, M.mat_array, sizeof(T) * 16);
}

//TL
template<class T>
matrix4<T>::matrix4(const matrix3<T>& M)
{
    memcpy(mat_array, M.mat_array, sizeof(T) * 3);
    mat_array[3] = 0.0;
    memcpy(mat_array+4, M.mat_array+3, sizeof(T) * 3);
    mat_array[7] = 0.0;
    memcpy(mat_array+8, M.mat_array+6, sizeof(T) * 3);
    mat_array[11] = 0.0f;
    mat_array[12] = 0.0f;
    mat_array[13] = 0.0f;
    mat_array[14] = 0.0f;
    mat_array[15] = 1.0f;
}

template<class T>
vector3<T> & cross(vector3<T> & u, const vector3<T> & v, const vector3<T> & w)
{
    u.x = v.y*w.z - v.z*w.y;
    u.y = v.z*w.x - v.x*w.z;
    u.z = v.x*w.y - v.y*w.x;
    return u;
}

template<class T>
T & dot(T& u, const vector3<T>& v, const vector3<T>& w)
{
    u = v.x*w.x + v.y*w.y + v.z*w.z;
    return u;
}

template<class T>
T dot(const vector2<T>& v, const vector2<T>& w)
{
    return v.x*w.x + v.y*w.y;
}

template<class T>
T dot(const vector3<T>& v, const vector3<T>& w)
{
    return v.x*w.x + v.y*w.y + v.z*w.z;
}

template<class T>
T & dot(T& u, const vector4<T>& v, const vector4<T>& w)
{
    u = v.x*w.x + v.y*w.y + v.z*w.z + v.w*w.w;
    return u;
}

template<class T>
T dot(const vector4<T>& v, const vector4<T>& w)
{
    return v.x*w.x + v.y*w.y + v.z*w.z + v.w*w.w;
}

template<class T>
T & dot(T& u, const vector3<T>& v, const vector4<T>& w)
{
    u = v.x*w.x + v.y*w.y + v.z*w.z;
    return u;
}

template<class T>
T dot(const vector3<T>& v, const vector4<T>& w)
{
    return v.x*w.x + v.y*w.y + v.z*w.z;
}

template<class T>
T & dot(T& u, const vector4<T>& v, const vector3<T>& w)
{
    u = v.x*w.x + v.y*w.y + v.z*w.z;
    return u;
}

template<class T>
T dot(const vector4<T>& v, const vector3<T>& w)
{
    return v.x*w.x + v.y*w.y + v.z*w.z;
}

template<class T>
vector3<T> & reflect(vector3<T>& r, const vector3<T>& n, const vector3<T>& l)
{
    T n_dot_l;
    n_dot_l = nv_two * dot(n_dot_l,n,l);
    mult<T>(r,l,-nv_one);
    madd(r,n,n_dot_l);
    return r;
}

template<class T>
vector3<T> & madd(vector3<T> & u, const vector3<T>& v, const T& lambda)
{
    u.x += v.x*lambda;
    u.y += v.y*lambda;
    u.z += v.z*lambda;
    return u;
}

template<class T>
vector3<T> & mult(vector3<T> & u, const vector3<T>& v, const T& lambda)
{
    u.x = v.x*lambda;
    u.y = v.y*lambda;
    u.z = v.z*lambda;
    return u;
}

template<class T>
vector3<T> & mult(vector3<T> & u, const vector3<T>& v, const vector3<T>& w)
{
    u.x = v.x*w.x;
    u.y = v.y*w.y;
    u.z = v.z*w.z;
    return u;
}

template<class T>
vector3<T> & sub(vector3<T> & u, const vector3<T>& v, const vector3<T>& w)
{
    u.x = v.x - w.x;
    u.y = v.y - w.y;
    u.z = v.z - w.z;
    return u;
}

template<class T>
vector3<T> & add(vector3<T> & u, const vector3<T>& v, const vector3<T>& w)
{
    u.x = v.x + w.x;
    u.y = v.y + w.y;
    u.z = v.z + w.z;
    return u;
}

template<class T>
void vector3<T>::orthogonalize( const vector3<T>& v )
{
	//  determine the orthogonal projection of this on v : dot( v , this ) * v
	//  and subtract it from this resulting in the orthogonalized this
	vector3<T> res = dot( v, vector3<T>(x, y, z) ) * v;
	x -= res.x;
	y -= res.y;
	z -= res.y;
}

template<class T>
vector3<T> & vector3<T>::rotateBy(const quaternion<T>& q)
{
    matrix3<T> M;
    quat_2_mat(M, q );
    vector3<T> dst;
    mult<T>(dst, M, *this);
    x = dst.x;
    y = dst.y;
    z = dst.z;
    return (*this);
}

template<class T>
T vector3<T>::normalize()
{
	T norm = sqrtf(x * x + y * y + z * z);
	if (norm > nv_eps)
		norm = nv_one / norm;
	else
		norm = nv_zero;
	x *= norm;
	y *= norm;
	z *= norm;
	return norm;
}

template<class T>
vector2<T> & scale(vector2<T>& u, const T s)
{
    u.x *= s;
    u.y *= s;
    return u;
}

template<class T>
vector3<T> & scale(vector3<T>& u, const T s)
{
    u.x *= s;
    u.y *= s;
    u.z *= s;
    return u;
}

template<class T>
vector4<T> & scale(vector4<T>& u, const T s)
{
    u.x *= s;
    u.y *= s;
    u.z *= s;
    u.w *= s;
    return u;
}

template<class T>
vector3<T> & mult(vector3<T>& u, const matrix3<T>& M, const vector3<T>& v)
{
    u.x = M.a00 * v.x + M.a01 * v.y + M.a02 * v.z;
    u.y = M.a10 * v.x + M.a11 * v.y + M.a12 * v.z;
    u.z = M.a20 * v.x + M.a21 * v.y + M.a22 * v.z;
    return u;
}

template<class T>
vector3<T> & mult(vector3<T>& u, const vector3<T>& v, const matrix3<T>& M)
{
    u.x = M.a00 * v.x + M.a10 * v.y + M.a20 * v.z;
    u.y = M.a01 * v.x + M.a11 * v.y + M.a21 * v.z;
    u.z = M.a02 * v.x + M.a12 * v.y + M.a22 * v.z;
    return u;
}

template<class T>
const vector3<T> operator*(const matrix3<T>& M, const vector3<T>& v)
{
	vector3<T> u;
    u.x = M.a00 * v.x + M.a01 * v.y + M.a02 * v.z;
    u.y = M.a10 * v.x + M.a11 * v.y + M.a12 * v.z;
    u.z = M.a20 * v.x + M.a21 * v.y + M.a22 * v.z;
    return u;
}

template<class T>
const vector3<T> operator*(const vector3<T>& v, const matrix3<T>& M)
{
	vector3<T> u;
    u.x = M.a00 * v.x + M.a10 * v.y + M.a20 * v.z;
    u.y = M.a01 * v.x + M.a11 * v.y + M.a21 * v.z;
    u.z = M.a02 * v.x + M.a12 * v.y + M.a22 * v.z;
    return u;
}

template<class T>
vector4<T> & mult(vector4<T>& u, const matrix4<T>& M, const vector4<T>& v)
{
    u.x = M.a00 * v.x + M.a01 * v.y + M.a02 * v.z + M.a03 * v.w;
    u.y = M.a10 * v.x + M.a11 * v.y + M.a12 * v.z + M.a13 * v.w;
    u.z = M.a20 * v.x + M.a21 * v.y + M.a22 * v.z + M.a23 * v.w;
    u.w = M.a30 * v.x + M.a31 * v.y + M.a32 * v.z + M.a33 * v.w;
    return u;
}

template<class T>
vector4<T> & mult(vector4<T>& u, const vector4<T>& v, const matrix4<T>& M)
{
    u.x = M.a00 * v.x + M.a10 * v.y + M.a20 * v.z + M.a30 * v.w;
    u.y = M.a01 * v.x + M.a11 * v.y + M.a21 * v.z + M.a31 * v.w;
    u.z = M.a02 * v.x + M.a12 * v.y + M.a22 * v.z + M.a32 * v.w;
    u.w = M.a03 * v.x + M.a13 * v.y + M.a23 * v.z + M.a33 * v.w;
    return u;
}

template<class T>
const vector4<T> operator*(const matrix4<T>& M, const vector4<T>& v)
{
	vector4<T> u;
    u.x = M.a00 * v.x + M.a01 * v.y + M.a02 * v.z + M.a03 * v.w;
    u.y = M.a10 * v.x + M.a11 * v.y + M.a12 * v.z + M.a13 * v.w;
    u.z = M.a20 * v.x + M.a21 * v.y + M.a22 * v.z + M.a23 * v.w;
    u.w = M.a30 * v.x + M.a31 * v.y + M.a32 * v.z + M.a33 * v.w;
    return u;
}

template<class T>
const vector4<T> operator*(const matrix4<T>& M, const vector3<T>& v)
{
	vector4<T> u;
    u.x = M.a00 * v.x + M.a01 * v.y + M.a02 * v.z + M.a03;
    u.y = M.a10 * v.x + M.a11 * v.y + M.a12 * v.z + M.a13;
    u.z = M.a20 * v.x + M.a21 * v.y + M.a22 * v.z + M.a23;
    u.w = M.a30 * v.x + M.a31 * v.y + M.a32 * v.z + M.a33;
    return u;
}

template<class T>
const vector4<T> operator*(const vector4<T>& v, const matrix4<T>& M)
{
	vector4<T> u;
    u.x = M.a00 * v.x + M.a10 * v.y + M.a20 * v.z + M.a30 * v.w;
    u.y = M.a01 * v.x + M.a11 * v.y + M.a21 * v.z + M.a31 * v.w;
    u.z = M.a02 * v.x + M.a12 * v.y + M.a22 * v.z + M.a32 * v.w;
    u.w = M.a03 * v.x + M.a13 * v.y + M.a23 * v.z + M.a33 * v.w;
    return u;
}

template<class T>
vector3<T> & mult_pos(vector3<T>& u, const matrix4<T>& M, const vector3<T>& v)
{
    T oow;
    T divider = v.x * M.a30 + v.y * M.a31 + v.z * M.a32 + M.a33;
    if (divider < nv_eps && divider > -nv_eps)
        oow = nv_one ;
    else
        oow = nv_one / divider;
    u.x = (M.a00 * v.x + M.a01 * v.y + M.a02 * v.z + M.a03) * oow;
    u.y = (M.a10 * v.x + M.a11 * v.y + M.a12 * v.z + M.a13) * oow;
    u.z = (M.a20 * v.x + M.a21 * v.y + M.a22 * v.z + M.a23) * oow;
    return u;
}

template<class T>
vector3<T> & mult_pos(vector3<T>& u, const vector3<T>& v, const matrix4<T>& M)
{
    T oow;
    T divider = v.x * M.a03 + v.y * M.a13 + v.z * M.a23 + M.a33;
    if (divider < nv_eps && divider > -nv_eps)
        oow = nv_one ;
    else
        oow = nv_one / divider;

    u.x = (M.a00 * v.x + M.a10 * v.y + M.a20 * v.z + M.a30) * oow;
    u.y = (M.a01 * v.x + M.a11 * v.y + M.a21 * v.z + M.a31) * oow;
    u.z = (M.a02 * v.x + M.a12 * v.y + M.a22 * v.z + M.a32) * oow;
    return u;
}

template<class T>
vector3<T> & mult_dir(vector3<T>& u, const matrix4<T>& M, const vector3<T>& v)
{
    u.x = M.a00 * v.x + M.a01 * v.y + M.a02 * v.z;
    u.y = M.a10 * v.x + M.a11 * v.y + M.a12 * v.z;
    u.z = M.a20 * v.x + M.a21 * v.y + M.a22 * v.z;
    return u;
}

template<class T>
vector3<T> & mult_dir(vector3<T>& u, const vector3<T>& v, const matrix4<T>& M)
{
    u.x = M.a00 * v.x + M.a10 * v.y + M.a20 * v.z;
    u.y = M.a01 * v.x + M.a11 * v.y + M.a21 * v.z;
    u.z = M.a02 * v.x + M.a12 * v.y + M.a22 * v.z;
    return u;
}

template<class T>
vector3<T> & mult(vector3<T>& u, const matrix4<T>& M, const vector3<T>& v)
{
    u.x = M.a00 * v.x + M.a01 * v.y + M.a02 * v.z + M.a03;
    u.y = M.a10 * v.x + M.a11 * v.y + M.a12 * v.z + M.a13;
    u.z = M.a20 * v.x + M.a21 * v.y + M.a22 * v.z + M.a23;
    return u;
}

template<class T>
vector3<T> & mult(vector3<T>& u, const vector3<T>& v, const matrix4<T>& M)
{
    u.x = M.a00 * v.x + M.a10 * v.y + M.a20 * v.z + M.a30;
    u.y = M.a01 * v.x + M.a11 * v.y + M.a21 * v.z + M.a31;
    u.z = M.a02 * v.x + M.a12 * v.y + M.a22 * v.z + M.a32;
    return u;
}

template<class T>
matrix4<T> & add(matrix4<T>& A, const matrix4<T>& B)
{
    A.a00 += B.a00;
    A.a10 += B.a10;
    A.a20 += B.a20;
    A.a30 += B.a30;
    A.a01 += B.a01;
    A.a11 += B.a11;
    A.a21 += B.a21;
    A.a31 += B.a31;
    A.a02 += B.a02;
    A.a12 += B.a12;
    A.a22 += B.a22;
    A.a32 += B.a32;
    A.a03 += B.a03;
    A.a13 += B.a13;
    A.a23 += B.a23;
    A.a33 += B.a33;
    return A;
}

template<class T>
matrix3<T> & add(matrix3<T>& A, const matrix3<T>& B)
{
    A.a00 += B.a00;
    A.a10 += B.a10;
    A.a20 += B.a20;
    A.a01 += B.a01;
    A.a11 += B.a11;
    A.a21 += B.a21;
    A.a02 += B.a02;
    A.a12 += B.a12;
    A.a22 += B.a22;
    return A;
}


// Computes C = A + B
template<class T>
matrix4<T> & add(matrix4<T> & C, const matrix4<T> & A, const matrix4<T> & B)
{
                                // If there is selfassignment involved
                                // we can't go without a temporary.
    if (&C == &A || &C == &B)
    {
        matrix4<T> mTemp;

        mTemp.a00 = A.a00 + B.a00;
        mTemp.a01 = A.a01 + B.a01;
        mTemp.a02 = A.a02 + B.a02;
        mTemp.a03 = A.a03 + B.a03;
        mTemp.a10 = A.a10 + B.a10;
        mTemp.a11 = A.a11 + B.a11;
        mTemp.a12 = A.a12 + B.a12;
        mTemp.a13 = A.a13 + B.a13;
        mTemp.a20 = A.a20 + B.a20;
        mTemp.a21 = A.a21 + B.a21;
        mTemp.a22 = A.a22 + B.a22;
        mTemp.a23 = A.a23 + B.a23;
        mTemp.a30 = A.a30 + B.a30;
        mTemp.a31 = A.a31 + B.a31;
        mTemp.a32 = A.a32 + B.a32;
        mTemp.a33 = A.a33 + B.a33;
    
        C = mTemp;
    }
    else
    {
        C.a00 = A.a00 + B.a00;
        C.a01 = A.a01 + B.a01;
        C.a02 = A.a02 + B.a02;
        C.a03 = A.a03 + B.a03;
        C.a10 = A.a10 + B.a10;
        C.a11 = A.a11 + B.a11;
        C.a12 = A.a12 + B.a12;
        C.a13 = A.a13 + B.a13;
        C.a20 = A.a20 + B.a20;
        C.a21 = A.a21 + B.a21;
        C.a22 = A.a22 + B.a22;
        C.a23 = A.a23 + B.a23;
        C.a30 = A.a30 + B.a30;
        C.a31 = A.a31 + B.a31;
        C.a32 = A.a32 + B.a32;
        C.a33 = A.a33 + B.a33;
    }
    return C;
}

template<class T>
matrix3<T> & add(matrix3<T> & C, const matrix3<T> & A, const matrix3<T> & B)
{
                                // If there is selfassignment involved
                                // we can't go without a temporary.
    if (&C == &A || &C == &B)
    {
        matrix3<T> mTemp;

        mTemp.a00 = A.a00 + B.a00;
        mTemp.a01 = A.a01 + B.a01;
        mTemp.a02 = A.a02 + B.a02;
        mTemp.a10 = A.a10 + B.a10;
        mTemp.a11 = A.a11 + B.a11;
        mTemp.a12 = A.a12 + B.a12;
        mTemp.a20 = A.a20 + B.a20;
        mTemp.a21 = A.a21 + B.a21;
        mTemp.a22 = A.a22 + B.a22;
   
        C = mTemp;
    }
    else
    {
        C.a00 = A.a00 + B.a00;
        C.a01 = A.a01 + B.a01;
        C.a02 = A.a02 + B.a02;
        C.a10 = A.a10 + B.a10;
        C.a11 = A.a11 + B.a11;
        C.a12 = A.a12 + B.a12;
        C.a20 = A.a20 + B.a20;
        C.a21 = A.a21 + B.a21;
        C.a22 = A.a22 + B.a22;
    }
    return C;
}


// C = A * B

// C.a00 C.a01 C.a02 C.a03   A.a00 A.a01 A.a02 A.a03   B.a00 B.a01 B.a02 B.a03
//                                                                            
// C.a10 C.a11 C.a12 C.a13   A.a10 A.a11 A.a12 A.a13   B.a10 B.a11 B.a12 B.a13
//                                                                         
// C.a20 C.a21 C.a22 C.a23   A.a20 A.a21 A.a22 A.a23   B.a20 B.a21 B.a22 B.a23  
//                                                                            
// C.a30 C.a31 C.a32 C.a33 = A.a30 A.a31 A.a32 A.a33 * B.a30 B.a31 B.a32 B.a33

template<class T>
matrix4<T> & mult(matrix4<T>& C, const matrix4<T>& A, const matrix4<T>& B)
{
                                // If there is selfassignment involved
                                // we can't go without a temporary.
    if (&C == &A || &C == &B)
    {
        matrix4<T> mTemp;

        mTemp.a00 = A.a00 * B.a00 + A.a01 * B.a10 + A.a02 * B.a20 + A.a03 * B.a30;
        mTemp.a10 = A.a10 * B.a00 + A.a11 * B.a10 + A.a12 * B.a20 + A.a13 * B.a30;
        mTemp.a20 = A.a20 * B.a00 + A.a21 * B.a10 + A.a22 * B.a20 + A.a23 * B.a30;
        mTemp.a30 = A.a30 * B.a00 + A.a31 * B.a10 + A.a32 * B.a20 + A.a33 * B.a30;
        mTemp.a01 = A.a00 * B.a01 + A.a01 * B.a11 + A.a02 * B.a21 + A.a03 * B.a31;
        mTemp.a11 = A.a10 * B.a01 + A.a11 * B.a11 + A.a12 * B.a21 + A.a13 * B.a31;
        mTemp.a21 = A.a20 * B.a01 + A.a21 * B.a11 + A.a22 * B.a21 + A.a23 * B.a31;
        mTemp.a31 = A.a30 * B.a01 + A.a31 * B.a11 + A.a32 * B.a21 + A.a33 * B.a31;
        mTemp.a02 = A.a00 * B.a02 + A.a01 * B.a12 + A.a02 * B.a22 + A.a03 * B.a32;
        mTemp.a12 = A.a10 * B.a02 + A.a11 * B.a12 + A.a12 * B.a22 + A.a13 * B.a32;
        mTemp.a22 = A.a20 * B.a02 + A.a21 * B.a12 + A.a22 * B.a22 + A.a23 * B.a32;
        mTemp.a32 = A.a30 * B.a02 + A.a31 * B.a12 + A.a32 * B.a22 + A.a33 * B.a32;
        mTemp.a03 = A.a00 * B.a03 + A.a01 * B.a13 + A.a02 * B.a23 + A.a03 * B.a33;
        mTemp.a13 = A.a10 * B.a03 + A.a11 * B.a13 + A.a12 * B.a23 + A.a13 * B.a33;
        mTemp.a23 = A.a20 * B.a03 + A.a21 * B.a13 + A.a22 * B.a23 + A.a23 * B.a33;
        mTemp.a33 = A.a30 * B.a03 + A.a31 * B.a13 + A.a32 * B.a23 + A.a33 * B.a33;
    
        C = mTemp;
    }
    else
    {
        C.a00 = A.a00 * B.a00 + A.a01 * B.a10 + A.a02 * B.a20 + A.a03 * B.a30;
        C.a10 = A.a10 * B.a00 + A.a11 * B.a10 + A.a12 * B.a20 + A.a13 * B.a30;
        C.a20 = A.a20 * B.a00 + A.a21 * B.a10 + A.a22 * B.a20 + A.a23 * B.a30;
        C.a30 = A.a30 * B.a00 + A.a31 * B.a10 + A.a32 * B.a20 + A.a33 * B.a30;
        C.a01 = A.a00 * B.a01 + A.a01 * B.a11 + A.a02 * B.a21 + A.a03 * B.a31;
        C.a11 = A.a10 * B.a01 + A.a11 * B.a11 + A.a12 * B.a21 + A.a13 * B.a31;
        C.a21 = A.a20 * B.a01 + A.a21 * B.a11 + A.a22 * B.a21 + A.a23 * B.a31;
        C.a31 = A.a30 * B.a01 + A.a31 * B.a11 + A.a32 * B.a21 + A.a33 * B.a31;
        C.a02 = A.a00 * B.a02 + A.a01 * B.a12 + A.a02 * B.a22 + A.a03 * B.a32;
        C.a12 = A.a10 * B.a02 + A.a11 * B.a12 + A.a12 * B.a22 + A.a13 * B.a32;
        C.a22 = A.a20 * B.a02 + A.a21 * B.a12 + A.a22 * B.a22 + A.a23 * B.a32;
        C.a32 = A.a30 * B.a02 + A.a31 * B.a12 + A.a32 * B.a22 + A.a33 * B.a32;
        C.a03 = A.a00 * B.a03 + A.a01 * B.a13 + A.a02 * B.a23 + A.a03 * B.a33;
        C.a13 = A.a10 * B.a03 + A.a11 * B.a13 + A.a12 * B.a23 + A.a13 * B.a33;
        C.a23 = A.a20 * B.a03 + A.a21 * B.a13 + A.a22 * B.a23 + A.a23 * B.a33;
        C.a33 = A.a30 * B.a03 + A.a31 * B.a13 + A.a32 * B.a23 + A.a33 * B.a33;
    }

    return C;
}

template<class T>
matrix4<T> matrix4<T>::operator*(const matrix4<T>& B) const
{
    matrix4<T> C;
    C.a00 = a00 * B.a00 + a01 * B.a10 + a02 * B.a20 + a03 * B.a30;
    C.a10 = a10 * B.a00 + a11 * B.a10 + a12 * B.a20 + a13 * B.a30;
    C.a20 = a20 * B.a00 + a21 * B.a10 + a22 * B.a20 + a23 * B.a30;
    C.a30 = a30 * B.a00 + a31 * B.a10 + a32 * B.a20 + a33 * B.a30;
    C.a01 = a00 * B.a01 + a01 * B.a11 + a02 * B.a21 + a03 * B.a31;
    C.a11 = a10 * B.a01 + a11 * B.a11 + a12 * B.a21 + a13 * B.a31;
    C.a21 = a20 * B.a01 + a21 * B.a11 + a22 * B.a21 + a23 * B.a31;
    C.a31 = a30 * B.a01 + a31 * B.a11 + a32 * B.a21 + a33 * B.a31;
    C.a02 = a00 * B.a02 + a01 * B.a12 + a02 * B.a22 + a03 * B.a32;
    C.a12 = a10 * B.a02 + a11 * B.a12 + a12 * B.a22 + a13 * B.a32;
    C.a22 = a20 * B.a02 + a21 * B.a12 + a22 * B.a22 + a23 * B.a32;
    C.a32 = a30 * B.a02 + a31 * B.a12 + a32 * B.a22 + a33 * B.a32;
    C.a03 = a00 * B.a03 + a01 * B.a13 + a02 * B.a23 + a03 * B.a33;
    C.a13 = a10 * B.a03 + a11 * B.a13 + a12 * B.a23 + a13 * B.a33;
    C.a23 = a20 * B.a03 + a21 * B.a13 + a22 * B.a23 + a23 * B.a33;
    C.a33 = a30 * B.a03 + a31 * B.a13 + a32 * B.a23 + a33 * B.a33;
    return C;
}

// C = A * B

// C.a00 C.a01 C.a02   A.a00 A.a01 A.a02   B.a00 B.a01 B.a02
//                                                          
// C.a10 C.a11 C.a12   A.a10 A.a11 A.a12   B.a10 B.a11 B.a12
//                                                          
// C.a20 C.a21 C.a22 = A.a20 A.a21 A.a22 * B.a20 B.a21 B.a22

template<class T>
matrix3<T> & mult(matrix3<T>& C, const matrix3<T>& A, const matrix3<T>& B)
{
                                // If there is sel fassignment involved
                                // we can't go without a temporary.
    if (&C == &A || &C == &B)
    {
        matrix3<T> mTemp;

        mTemp.a00 = A.a00 * B.a00 + A.a01 * B.a10 + A.a02 * B.a20;
        mTemp.a10 = A.a10 * B.a00 + A.a11 * B.a10 + A.a12 * B.a20;
        mTemp.a20 = A.a20 * B.a00 + A.a21 * B.a10 + A.a22 * B.a20;
        mTemp.a01 = A.a00 * B.a01 + A.a01 * B.a11 + A.a02 * B.a21;
        mTemp.a11 = A.a10 * B.a01 + A.a11 * B.a11 + A.a12 * B.a21;
        mTemp.a21 = A.a20 * B.a01 + A.a21 * B.a11 + A.a22 * B.a21;
        mTemp.a02 = A.a00 * B.a02 + A.a01 * B.a12 + A.a02 * B.a22;
        mTemp.a12 = A.a10 * B.a02 + A.a11 * B.a12 + A.a12 * B.a22;
        mTemp.a22 = A.a20 * B.a02 + A.a21 * B.a12 + A.a22 * B.a22;

        C = mTemp;
    }
    else
    {
        C.a00 = A.a00 * B.a00 + A.a01 * B.a10 + A.a02 * B.a20;
        C.a10 = A.a10 * B.a00 + A.a11 * B.a10 + A.a12 * B.a20;
        C.a20 = A.a20 * B.a00 + A.a21 * B.a10 + A.a22 * B.a20;
        C.a01 = A.a00 * B.a01 + A.a01 * B.a11 + A.a02 * B.a21;
        C.a11 = A.a10 * B.a01 + A.a11 * B.a11 + A.a12 * B.a21;
        C.a21 = A.a20 * B.a01 + A.a21 * B.a11 + A.a22 * B.a21;
        C.a02 = A.a00 * B.a02 + A.a01 * B.a12 + A.a02 * B.a22;
        C.a12 = A.a10 * B.a02 + A.a11 * B.a12 + A.a12 * B.a22;
        C.a22 = A.a20 * B.a02 + A.a21 * B.a12 + A.a22 * B.a22;
    }

    return C;
}


template<class T>
matrix3<T> & transpose(matrix3<T>& A)
{
    T tmp;
    tmp = A.a01;
    A.a01 = A.a10;
    A.a10 = tmp;

    tmp = A.a02;
    A.a02 = A.a20;
    A.a20 = tmp;

    tmp = A.a12;
    A.a12 = A.a21;
    A.a21 = tmp;
    return A;
}

template<class T>
matrix4<T> & transpose(matrix4<T>& A)
{
    T tmp;
    tmp = A.a01;
    A.a01 = A.a10;
    A.a10 = tmp;

    tmp = A.a02;
    A.a02 = A.a20;
    A.a20 = tmp;

    tmp = A.a03;
    A.a03 = A.a30;
    A.a30 = tmp;

    tmp = A.a12;
    A.a12 = A.a21;
    A.a21 = tmp;

    tmp = A.a13;
    A.a13 = A.a31;
    A.a31 = tmp;

    tmp = A.a23;
    A.a23 = A.a32;
    A.a32 = tmp;
    return A;
}

template<class T>
matrix4<T> & transpose(matrix4<T>& B, const matrix4<T>& A)
{
    B.a00 = A.a00;
    B.a01 = A.a10;
    B.a02 = A.a20;
    B.a03 = A.a30;
    B.a10 = A.a01;
    B.a11 = A.a11;
    B.a12 = A.a21;
    B.a13 = A.a31;
    B.a20 = A.a02;
    B.a21 = A.a12;
    B.a22 = A.a22;
    B.a23 = A.a32;
    B.a30 = A.a03;
    B.a31 = A.a13;
    B.a32 = A.a23;
    B.a33 = A.a33;
    return B;
}

template<class T>
matrix3<T> & transpose(matrix3<T>& B, const matrix3<T>& A)
{
    B.a00 = A.a00;
    B.a01 = A.a10;
    B.a02 = A.a20;
    B.a10 = A.a01;
    B.a11 = A.a11;
    B.a12 = A.a21;
    B.a20 = A.a02;
    B.a21 = A.a12;
    B.a22 = A.a22;
    return B;
}

/*
    calculate the determinent of a 2x2 matrix in the from

    | a1 a2 |
    | b1 b2 |

*/
template<class T>
T det2x2(T a1, T a2, T b1, T b2)
{
    return a1 * b2 - b1 * a2;
}

/*
    calculate the determinent of a 3x3 matrix in the from

    | a1 a2 a3 |
    | b1 b2 b3 |
    | c1 c2 c3 |

*/
template<class T>
T det3x3(T a1, T a2, T a3, 
                         T b1, T b2, T b3, 
                         T c1, T c2, T c3)
{
    return a1 * det2x2(b2, b3, c2, c3) - b1 * det2x2(a2, a3, c2, c3) + c1 * det2x2(a2, a3, b2, b3);
}

template<class T>
matrix4<T> & invert(matrix4<T>& B, const matrix4<T>& A)
{
    T det,oodet;

    B.a00 =  det3x3(A.a11, A.a21, A.a31, A.a12, A.a22, A.a32, A.a13, A.a23, A.a33);
    B.a10 = -det3x3(A.a10, A.a20, A.a30, A.a12, A.a22, A.a32, A.a13, A.a23, A.a33);
    B.a20 =  det3x3(A.a10, A.a20, A.a30, A.a11, A.a21, A.a31, A.a13, A.a23, A.a33);
    B.a30 = -det3x3(A.a10, A.a20, A.a30, A.a11, A.a21, A.a31, A.a12, A.a22, A.a32);

    B.a01 = -det3x3(A.a01, A.a21, A.a31, A.a02, A.a22, A.a32, A.a03, A.a23, A.a33);
    B.a11 =  det3x3(A.a00, A.a20, A.a30, A.a02, A.a22, A.a32, A.a03, A.a23, A.a33);
    B.a21 = -det3x3(A.a00, A.a20, A.a30, A.a01, A.a21, A.a31, A.a03, A.a23, A.a33);
    B.a31 =  det3x3(A.a00, A.a20, A.a30, A.a01, A.a21, A.a31, A.a02, A.a22, A.a32);

    B.a02 =  det3x3(A.a01, A.a11, A.a31, A.a02, A.a12, A.a32, A.a03, A.a13, A.a33);
    B.a12 = -det3x3(A.a00, A.a10, A.a30, A.a02, A.a12, A.a32, A.a03, A.a13, A.a33);
    B.a22 =  det3x3(A.a00, A.a10, A.a30, A.a01, A.a11, A.a31, A.a03, A.a13, A.a33);
    B.a32 = -det3x3(A.a00, A.a10, A.a30, A.a01, A.a11, A.a31, A.a02, A.a12, A.a32);

    B.a03 = -det3x3(A.a01, A.a11, A.a21, A.a02, A.a12, A.a22, A.a03, A.a13, A.a23);
    B.a13 =  det3x3(A.a00, A.a10, A.a20, A.a02, A.a12, A.a22, A.a03, A.a13, A.a23);
    B.a23 = -det3x3(A.a00, A.a10, A.a20, A.a01, A.a11, A.a21, A.a03, A.a13, A.a23);
    B.a33 =  det3x3(A.a00, A.a10, A.a20, A.a01, A.a11, A.a21, A.a02, A.a12, A.a22);

    det = (A.a00 * B.a00) + (A.a01 * B.a10) + (A.a02 * B.a20) + (A.a03 * B.a30);

                                // The following divions goes unchecked for division
                                // by zero. We should consider throwing an exception
                                // if det < eps.
    oodet = nv_one / det;

    B.a00 *= oodet;
    B.a10 *= oodet;
    B.a20 *= oodet;
    B.a30 *= oodet;

    B.a01 *= oodet;
    B.a11 *= oodet;
    B.a21 *= oodet;
    B.a31 *= oodet;

    B.a02 *= oodet;
    B.a12 *= oodet;
    B.a22 *= oodet;
    B.a32 *= oodet;

    B.a03 *= oodet;
    B.a13 *= oodet;
    B.a23 *= oodet;
    B.a33 *= oodet;

    return B;
}

template<class T>
matrix4<T> & invert_rot_trans(matrix4<T>& B, const matrix4<T>& A)
{
    B.a00 = A.a00;
    B.a10 = A.a01;
    B.a20 = A.a02;
    B.a30 = A.a30;
    B.a01 = A.a10;
    B.a11 = A.a11;
    B.a21 = A.a12;
    B.a31 = A.a31;
    B.a02 = A.a20;
    B.a12 = A.a21;
    B.a22 = A.a22;
    B.a32 = A.a32;
    B.a03 = - (A.a00 * A.a03 + A.a10 * A.a13 + A.a20 * A.a23);
    B.a13 = - (A.a01 * A.a03 + A.a11 * A.a13 + A.a21 * A.a23);
    B.a23 = - (A.a02 * A.a03 + A.a12 * A.a13 + A.a22 * A.a23);
    B.a33 = A.a33;
    return B;
}

template<class T>
T det(const matrix3<T>& A)
{
    return det3x3(A.a00, A.a01, A.a02, 
                 A.a10, A.a11, A.a12, 
                 A.a20, A.a21, A.a22);
}

template<class T>
matrix3<T> & invert(matrix3<T>& B, const matrix3<T>& A)
{
    T det,oodet;

    B.a00 =  (A.a11 * A.a22 - A.a21 * A.a12);
    B.a10 = -(A.a10 * A.a22 - A.a20 * A.a12);
    B.a20 =  (A.a10 * A.a21 - A.a20 * A.a11);
    B.a01 = -(A.a01 * A.a22 - A.a21 * A.a02);
    B.a11 =  (A.a00 * A.a22 - A.a20 * A.a02);
    B.a21 = -(A.a00 * A.a21 - A.a20 * A.a01);
    B.a02 =  (A.a01 * A.a12 - A.a11 * A.a02);
    B.a12 = -(A.a00 * A.a12 - A.a10 * A.a02);
    B.a22 =  (A.a00 * A.a11 - A.a10 * A.a01);

    det = (A.a00 * B.a00) + (A.a01 * B.a10) + (A.a02 * B.a20);
    
    oodet = nv_one / det;

    B.a00 *= oodet; B.a01 *= oodet; B.a02 *= oodet;
    B.a10 *= oodet; B.a11 *= oodet; B.a12 *= oodet;
    B.a20 *= oodet; B.a21 *= oodet; B.a22 *= oodet;
    return B;
}

template<class T>
vector2<T> & normalize(vector2<T>& u)
{
    T norm = sqrtf(u.x * u.x + u.y * u.y);
    if (norm > nv_eps)
        norm = nv_one / norm;
    else
        norm = nv_zero;
    return scale(u,norm); 
}

template<class T>
vector3<T> & normalize(vector3<T>& u)
{
    T norm = sqrtf(u.x * u.x + u.y * u.y + u.z * u.z);
    if (norm > nv_eps)
        norm = nv_one / norm;
    else
        norm = nv_zero;
    return scale(u,norm); 
}

template<class T>
vector4<T> & normalize(vector4<T>& u)
{
    T norm = sqrtf(u.x * u.x + u.y * u.y + u.z * u.z + u.w * u.w);
    if (norm > nv_eps)
        norm = nv_one / norm;
    else
        norm = nv_zero;
    return scale(u,norm); 
}

template<class T>
quaternion<T> & normalize(quaternion<T> & p)
{
    T norm = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z + p.w * p.w);
    if (norm > nv_eps)
        norm = nv_one / norm;
    else
        norm = nv_zero;
    p.x *= norm;
    p.y *= norm;
    p.z *= norm;
    p.w *= norm;
    return p; 
}

template<class T>
matrix4<T> & look_at(matrix4<T>& M, const vector3<T>& eye, const vector3<T>& center, const vector3<T>& up)
{
    vector3<T> x, y, z;

    // make rotation matrix

    // Z vector
    z.x = eye.x - center.x;
    z.y = eye.y - center.y;
    z.z = eye.z - center.z;
    normalize(z);

    // Y vector
    y.x = up.x;
    y.y = up.y;
    y.z = up.z;

    // X vector = Y cross Z
    cross(x,y,z);

    // Recompute Y = Z cross X
    cross(y,z,x);

    // cross product gives area of parallelogram, which is < 1.0 for
    // non-perpendicular unit-length vectors; so normalize x, y here
    normalize(x);
    normalize(y);

    M.a00 = x.x; M.a01 = x.y; M.a02 = x.z; M.a03 = -x.x * eye.x - x.y * eye.y - x.z*eye.z;
    M.a10 = y.x; M.a11 = y.y; M.a12 = y.z; M.a13 = -y.x * eye.x - y.y * eye.y - y.z*eye.z;
    M.a20 = z.x; M.a21 = z.y; M.a22 = z.z; M.a23 = -z.x * eye.x - z.y * eye.y - z.z*eye.z;
    M.a30 = nv_zero; M.a31 = nv_zero; M.a32 = nv_zero; M.a33 = nv_one;
    return M;
}

template<class T>
matrix4<T> & frustum(matrix4<T>& M, const T l, const T r, const T b, 
               const T t, const T n, const T f)
{
    M.a00 = (nv_two*n) / (r-l);
    M.a10 = 0.0;
    M.a20 = 0.0;
    M.a30 = 0.0;

    M.a01 = 0.0;
    M.a11 = (nv_two*n) / (t-b);
    M.a21 = 0.0;
    M.a31 = 0.0;

    M.a02 = (r+l) / (r-l);
    M.a12 = (t+b) / (t-b);
    M.a22 = -(f+n) / (f-n);
    M.a32 = -nv_one;

    M.a03 = 0.0;
    M.a13 = 0.0;
    M.a23 = -(nv_two*f*n) / (f-n);
    M.a33 = 0.0;
    return M;
}

template<class T>
matrix4<T> & perspective(matrix4<T>& M, const T fovy, const T aspect, const T n, const T f)
{
    T xmin, xmax, ymin, ymax;

    ymax = n * tanf(fovy * nv_to_rad * nv_one_half);
    ymin = -ymax;

    xmin = ymin * aspect;
    xmax = ymax * aspect;

    return frustum(M, xmin, xmax, ymin, ymax, n, f);
}

template<class T>
matrix4<T> & ortho(matrix4<T> & M, const T left, 
                              const T right, 
                              const T bottom, 
                              const T top,
                              const T n,
                              const T f)
{
    M.a00 = nv_two / (right - left);
    M.a01 = nv_zero;
    M.a02 = nv_zero;
    M.a03 = - (right + left) / (right - left);
    M.a10 = nv_zero;
    M.a11 = nv_two / (top - bottom);
    M.a12 = nv_zero;
    M.a13 = - (top + bottom) / (top - bottom);
    M.a20 = nv_zero;
    M.a21 = nv_zero;
    M.a22 = - nv_two / (f - n);
    M.a23 = - (f + n) / (f - n);
    M.a30 = nv_zero;
    M.a31 = nv_zero;
    M.a32 = nv_zero;
    M.a33 = nv_one;
    return M;
}

const quatf quatf::Identity(0, 0, 0, 1);

template<class T>
quaternion<T>::quaternion()
{
}

template<class T>
quaternion<T>::quaternion(T *q)
{
    x = q[0];
    y = q[1];
    z = q[2];
    w = q[3];
}

template<class T>
quaternion<T>::quaternion(T x, T y, T z, T w) : x(x), y(y), z(z), w(w)
{
}

template<class T>
quaternion<T>::quaternion(const quaternion<T>& quaternion)
{
	x = quaternion.x;
	y = quaternion.y;
	z = quaternion.z;
	w = quaternion.w;
}

template<class T>
quaternion<T>::quaternion(const vector3<T>& axis, T angle)
{
	T len = axis.norm();
	if (len) {
		T invLen = 1 / len;
		T angle2 = angle / 2;
		T scale = sinf(angle2) * invLen;
		x = scale * axis[0];
		y = scale * axis[1];
		z = scale * axis[2];
		w = cosf(angle2);
	}
}

template<class T>
quaternion<T>::quaternion(const matrix3<T>& rot)
{
	from_matrix(rot);
}
template<class T>
quaternion<T>::quaternion(const matrix4<T>& rot)
{
	from_matrix(rot);
}

template<class T>
quaternion<T>& quaternion<T>::operator=(const quaternion<T>& quaternion)
{
	x = quaternion.x;
	y = quaternion.y;
	z = quaternion.z;
	w = quaternion.w;
	return *this;
}

template<class T>
quaternion<T> quaternion<T>::inverse()
{
	return quaternion<T>(- x, - y, - z, w);
}
template<class T>
quaternion<T> quaternion<T>::conjugate()
{
	return quaternion<T>(- x, - y, - z, w);
}

template<class T>
void quaternion<T>::normalize()
{
	T len = sqrtf(x * x + y * y + z * z + w * w);
	if (len > 0) {
		T invLen = 1 / len;
		x *= invLen;
		y *= invLen;
		z *= invLen;
		w *= invLen;
	}
}

template<class T>
void quaternion<T>::from_matrix(const matrix3<T>& mat)
{
	T trace = mat(0, 0) + mat(1, 1) + mat(2, 2);
	if (trace > nv_zero) 
    {
		T scale = sqrtf(trace + nv_one);
		w = nv_one_half * scale;
		scale = nv_one_half / scale;
		x = scale * (mat(2, 1) - mat(1, 2));
		y = scale * (mat(0, 2) - mat(2, 0));
		z = scale * (mat(1, 0) - mat(0, 1));
	}
	else 
    {
		static int next[] = { 1, 2, 0 };
		int i = 0;
		if (mat(1, 1) > mat(0, 0))
			i = 1;
		if (mat(2, 2) > mat(i, i))
			i = 2;
		int j = next[i];
		int k = next[j];
		T scale = sqrtf(mat(i, i) - mat(j, j) - mat(k, k) + 1);
		T* q[] = { &x, &y, &z };
		*q[i] = 0.5f * scale;
		scale = 0.5f / scale;
		w = scale * (mat(k, j) - mat(j, k));
		*q[j] = scale * (mat(j, i) + mat(i, j));
		*q[k] = scale * (mat(k, i) + mat(i, k));
	}
}

template<class T>
void quaternion<T>::from_matrix(const matrix4<T>& mat)
{
	T trace = mat(0, 0) + mat(1, 1) + mat(2, 2);
	if (trace > nv_zero) 
    {
		T scale = sqrtf(trace + nv_one);
		w = nv_one_half * scale;
		scale = nv_one_half / scale;
		x = scale * (mat(2, 1) - mat(1, 2));
		y = scale * (mat(0, 2) - mat(2, 0));
		z = scale * (mat(1, 0) - mat(0, 1));
	}
	else 
    {
		static int next[] = { 1, 2, 0 };
		int i = 0;
		if (mat(1, 1) > mat(0, 0))
			i = 1;
		if (mat(2, 2) > mat(i, i))
			i = 2;
		int j = next[i];
		int k = next[j];
		T scale = sqrtf(mat(i, i) - mat(j, j) - mat(k, k) + 1);
		T* q[] = { &x, &y, &z };
		*q[i] = 0.5f * scale;
		scale = 0.5f / scale;
		w = scale * (mat(k, j) - mat(j, k));
		*q[j] = scale * (mat(j, i) + mat(i, j));
		*q[k] = scale * (mat(k, i) + mat(i, k));
	}
}

template<class T>
void quaternion<T>::to_matrix(matrix3<T>& mat) const
{
	T x2 = x * 2;
	T y2 = y * 2;
	T z2 = z * 2;
	T wx = x2 * w;
	T wy = y2 * w;
	T wz = z2 * w;
	T xx = x2 * x;
	T xy = y2 * x;
	T xz = z2 * x;
	T yy = y2 * y;
	T yz = z2 * y;
	T zz = z2 * z;
	mat(0, 0) = 1 - (yy + zz);
	mat(0, 1) = xy - wz;
	mat(0, 2) = xz + wy;
	mat(1, 0) = xy + wz;
	mat(1, 1) = 1 - (xx + zz);
	mat(1, 2) = yz - wx;
	mat(2, 0) = xz - wy;
	mat(2, 1) = yz + wx;
	mat(2, 2) = 1 - (xx + yy);
}

template<class T>
void quaternion<T>::to_matrix(matrix4<T>& mat) const
{
	T x2 = x * 2;
	T y2 = y * 2;
	T z2 = z * 2;
	T wx = x2 * w;
	T wy = y2 * w;
	T wz = z2 * w;
	T xx = x2 * x;
	T xy = y2 * x;
	T xz = z2 * x;
	T yy = y2 * y;
	T yz = z2 * y;
	T zz = z2 * z;
	mat(0, 0) = 1 - (yy + zz);
	mat(0, 1) = xy - wz;
	mat(0, 2) = xz + wy;
	mat(0, 3) = 0.0f;
	mat(1, 0) = xy + wz;
	mat(1, 1) = 1 - (xx + zz);
	mat(1, 2) = yz - wx;
	mat(1, 3) = 0.0f;
	mat(2, 0) = xz - wy;
	mat(2, 1) = yz + wx;
	mat(2, 2) = 1 - (xx + yy);
	mat(2, 3) = 0.0f;
	mat(3, 0) = 0.0f;
	mat(3, 1) = 0.0f;
	mat(3, 2) = 0.0f;
	mat(3, 3) = 1.0f;
}

template<class T>
const quaternion<T> operator*(const quaternion<T>& p, const quaternion<T>& q)
{
	return quaternion<T>(
		p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y,
		p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z,
		p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x,
		p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z
	);
}

template<class T>
quaternion<T>& quaternion<T>::operator*=(const quaternion<T>& q)
{
	*this = *this * q;
	return *this;
}

template<class T>
matrix3<T> & quat_2_mat(matrix3<T>& M, const quaternion<T>& q)
{
	q.to_matrix(M);
    return M;
}

template<class T>
quaternion<T> & mat_2_quat(quaternion<T>& q, const matrix3<T>& M)
{
	q.from_matrix(M);
    return q;
} 

template<class T>
quaternion<T> & mat_2_quat(quaternion<T>& q, const matrix4<T>& M)
{
	matrix3<T> m;
	M.get_rot(m);
	q.from_matrix(m);
    return q;
} 

/*
    Given an axis and angle, compute quaternion.
 */
template<class T>
quaternion<T> & axis_to_quat(quaternion<T>& q, const vector3<T>& a, const T phi)
{
    vector3<T> tmp(a.x, a.y, a.z);

    normalize(tmp);
	T s = sinf(phi/nv_two);
    q.x = s * tmp.x;
    q.y = s * tmp.y;
    q.z = s * tmp.z;
    q.w = cosf(phi/nv_two);
    return q;
}

template<class T>
quaternion<T> & conj(quaternion<T> & p)
{
    p.x = -p.x;
    p.y = -p.y;
    p.z = -p.z;
    return p;
}

template<class T>
 quaternion<T> & conj(quaternion<T>& p, const quaternion<T>& q)
{
    p.x = -q.x;
    p.y = -q.y;
    p.z = -q.z;
    p.w = q.w;
    return p;
}

template<class T>
 quaternion<T> & add_quats(quaternion<T>& p, const quaternion<T>& q1, const quaternion<T>& q2)
{
    quaternion<T> t1, t2;

    t1 = q1;
    t1.x *= q2.w;
    t1.y *= q2.w;
    t1.z *= q2.w;

    t2 = q2;
    t2.x *= q1.w;
    t2.y *= q1.w;
    t2.z *= q1.w;

    p.x = (q2.y * q1.z) - (q2.z * q1.y) + t1.x + t2.x;
    p.y = (q2.z * q1.x) - (q2.x * q1.z) + t1.y + t2.y;
    p.z = (q2.x * q1.y) - (q2.y * q1.x) + t1.z + t2.z;
    p.w = q1.w * q2.w - (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z);

    return p;
}

template<class T>
T & dot(T& s, const quaternion<T>& q1, const quaternion<T>& q2)
{
    s = q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
    return s;
}

template<class T>
T dot(const quaternion<T>& q1, const quaternion<T>& q2)
{
    return q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
}

#ifndef acosf
#define acosf acos
#endif

template<class T>
quaternion<T> & slerp_quats(quaternion<T> & p, T s, const quaternion<T> & q1, const quaternion<T> & q2)
{
    T cosine = dot(q1, q2);
	if (cosine < -1)
		cosine = -1;
	else if (cosine > 1)
		cosine = 1;
    T angle = (T)acosf(cosine);
    if (fabs(angle) < nv_eps) {
		p = q1;
        return p;
	}
    T sine = sinf(angle);
    T sineInv = 1.0f / sine;
    T c1 = sinf((1.0f - s) * angle) * sineInv;
    T c2 = sinf(s * angle) * sineInv;
	p.x = c1 * q1.x + c2 * q2.x;
	p.y = c1 * q1.y + c2 * q2.y;
	p.z = c1 * q1.z + c2 * q2.z;
	p.w = c1 * q1.w + c2 * q2.w;
    return p;
}

const int HALF_RAND = (RAND_MAX / 2);

template<class T>
 T nv_random()
{
	return ((T)(rand() - HALF_RAND) / (T)HALF_RAND);
}

// v is normalized
// theta in radians
template<class T>
matrix3<T> & matrix3<T>::set_rot(const T& theta, const vector3<T>& v) 
{
    T ct = T(cos(theta));
    T st = T(sin(theta));

    T xx = v.x * v.x;
    T yy = v.y * v.y;
    T zz = v.z * v.z;
    T xy = v.x * v.y;
    T xz = v.x * v.z;
    T yz = v.y * v.z;

    a00 = xx + ct*(1-xx);
    a01 = xy + ct*(-xy) + st*-v.z;
    a02 = xz + ct*(-xz) + st*v.y;

    a10 = xy + ct*(-xy) + st*v.z;
    a11 = yy + ct*(1-yy);
    a12 = yz + ct*(-yz) + st*-v.x;

    a20 = xz + ct*(-xz) + st*-v.y;
    a21 = yz + ct*(-yz) + st*v.x;
    a22 = zz + ct*(1-zz);
    return *this;
}

template<class T>
matrix3<T> & matrix3<T>::set_rot(const vector3<T>& u, const vector3<T>& v)
{
    T phi;
    T h;
    T lambda;
    vector3<T> w;

    cross(w,u,v);
    dot(phi,u,v);
    dot(lambda,w,w);
    if (lambda > nv_eps)
        h = (nv_one - phi) / lambda;
    else
        h = lambda;
    
    T hxy = w.x * w.y * h;
    T hxz = w.x * w.z * h;
    T hyz = w.y * w.z * h;

    a00 = phi + w.x * w.x * h;
    a01 = hxy - w.z;
    a02 = hxz + w.y;

    a10 = hxy + w.z;
    a11 = phi + w.y * w.y * h;
    a12 = hyz - w.x;

    a20 = hxz - w.y;
    a21 = hyz + w.x;
    a22 = phi + w.z * w.z * h;
    return *this;
}

template<class T>
T matrix3<T>::norm_one()
{
    T sum, max = nv_zero;
    sum = fabs(a00) + fabs(a10) + fabs(a20);
    if (max < sum)
        max = sum;
    sum = fabs(a01) + fabs(a11) + fabs(a21);
    if (max < sum)
        max = sum;
    sum = fabs(a02) + fabs(a12) + fabs(a22);
    if (max < sum)
        max = sum;
    return max;
}

template<class T>
T matrix3<T>::norm_inf()
{
    T sum, max = nv_zero;
    sum = fabs(a00) + fabs(a01) + fabs(a02);
    if (max < sum)
        max = sum;
    sum = fabs(a10) + fabs(a11) + fabs(a12);
    if (max < sum)
        max = sum;
    sum = fabs(a20) + fabs(a21) + fabs(a22);
    if (max < sum)
        max = sum;
    return max;
}

template<class T>
matrix4<T> & matrix4<T>::set_rot(const quaternion<T>& q)
{
	matrix3<T> m;
	q.to_matrix(m);
	set_rot(m);
    return *this;
}

// v is normalized
// theta in radians
template<class T>
matrix4<T> & matrix4<T>::set_rot(const T& theta, const vector3<T>& v) 
{
    T ct = T(cos(theta));
    T st = T(sin(theta));

    T xx = v.x * v.x;
    T yy = v.y * v.y;
    T zz = v.z * v.z;
    T xy = v.x * v.y;
    T xz = v.x * v.z;
    T yz = v.y * v.z;

    a00 = xx + ct*(1-xx);
    a01 = xy + ct*(-xy) + st*-v.z;
    a02 = xz + ct*(-xz) + st*v.y;

    a10 = xy + ct*(-xy) + st*v.z;
    a11 = yy + ct*(1-yy);
    a12 = yz + ct*(-yz) + st*-v.x;

    a20 = xz + ct*(-xz) + st*-v.y;
    a21 = yz + ct*(-yz) + st*v.x;
    a22 = zz + ct*(1-zz);
    return *this;
}

template<class T>
matrix4<T> & matrix4<T>::set_rot(const vector3<T>& u, const vector3<T>& v)
{
    T phi;
    T h;
    T lambda;
    vector3<T> w;

    cross(w,u,v);
    dot(phi,u,v);
    dot(lambda,w,w);
    if (lambda > nv_eps)
        h = (nv_one - phi) / lambda;
    else
        h = lambda;
    
    T hxy = w.x * w.y * h;
    T hxz = w.x * w.z * h;
    T hyz = w.y * w.z * h;

    a00 = phi + w.x * w.x * h;
    a01 = hxy - w.z;
    a02 = hxz + w.y;

    a10 = hxy + w.z;
    a11 = phi + w.y * w.y * h;
    a12 = hyz - w.x;

    a20 = hxz - w.y;
    a21 = hyz + w.x;
    a22 = phi + w.z * w.z * h;
    return *this;
}

template<class T>
matrix4<T> & matrix4<T>::set_rot(const matrix3<T>& M)
{
    // copy the 3x3 rotation block
    a00 = M.a00; a10 = M.a10; a20 = M.a20;
    a01 = M.a01; a11 = M.a11; a21 = M.a21;
    a02 = M.a02; a12 = M.a12; a22 = M.a22;
    return *this;
}

template<class T>
matrix4<T> & matrix4<T>::set_scale(const vector3<T>& s)
{
	a00 = s.x;
	a11 = s.y;
	a22 = s.z;
    return *this;
}

template<class T>
vector3<T>& matrix4<T>::get_scale(vector3<T>& s) const
{
	s.x = a00;
	s.y = a11;
	s.z = a22;
	return s;
}

template<class T>
matrix4<T> & matrix4<T>::as_scale(const vector3<T>& s)
{
    memcpy(mat_array, array16_id, sizeof(T) * 16);
    a00 = s.x;
    a11 = s.y;
    a22 = s.z;
    return *this;
}

template<class T>
matrix4<T> & matrix4<T>::as_scale(const T& s)
{
    memcpy(mat_array, array16_id, sizeof(T) * 16);
    a00 = s;
    a11 = s;
    a22 = s;
    return *this;
}

template<class T>
matrix4<T> & matrix4<T>::set_translation(const vector3<T>& t)
{
    a03 = t.x;
    a13 = t.y;
    a23 = t.z;
    return *this;
}

template<class T>
matrix4<T> & matrix4<T>::as_translation(const vector3<T>& t)
{
    memcpy(mat_array, array16_id, sizeof(T) * 16);
    a03 = t.x;
    a13 = t.y;
    a23 = t.z;
    return *this;
}

template<class T>
vector3<T> & matrix4<T>::get_translation(vector3<T>& t) const
{
    t.x = a03;
    t.y = a13;
    t.z = a23;
    return t;
}

template<class T>
matrix3<T> & matrix4<T>::get_rot(matrix3<T>& M) const
{
    // assign the 3x3 rotation block
    M.a00 = a00; M.a10 = a10; M.a20 = a20;
    M.a01 = a01; M.a11 = a11; M.a21 = a21;
    M.a02 = a02; M.a12 = a12; M.a22 = a22;
    return M;
}

template<class T>
quaternion<T> & matrix4<T>::get_rot(quaternion<T>& q) const
{
	matrix3<T> m;
	get_rot(m);
	q.from_matrix(m);
    return q;
}

template<class T>
matrix4<T> & negate(matrix4<T> & M)
{
    for (int i = 0; i < 16; ++i)
        M.mat_array[i]= -M.mat_array[i];
    return M;
}

template<class T>
matrix3<T> & negate(matrix3<T> & M)
{
    for (int i = 0; i < 9; ++i)
        M.mat_array[i]= -M.mat_array[i];
    return M;
}

template<class T>
matrix3<T>& tangent_basis(matrix3<T>& basis, const vector3<T>& v0, const vector3<T>& v1, const vector3<T>& v2, const vector2<T>& t0, const vector2<T>& t1, const vector2<T>& t2, const vector3<T> & n)
{
    vector3<T> cp;
    vector3<T> e0(v1.x - v0.x, t1.s - t0.s, t1.t - t0.t);
    vector3<T> e1(v2.x - v0.x, t2.s - t0.s, t2.t - t0.t);

    cross(cp,e0,e1);
    if ( fabs(cp.x) > nv_eps)
    {
        basis.a00 = -cp.y / cp.x;        
        basis.a10 = -cp.z / cp.x;
    }

    e0.x = v1.y - v0.y;
    e1.x = v2.y - v0.y;

    cross(cp,e0,e1);
    if ( fabs(cp.x) > nv_eps)
    {
        basis.a01 = -cp.y / cp.x;        
        basis.a11 = -cp.z / cp.x;
    }

    e0.x = v1.z - v0.z;
    e1.x = v2.z - v0.z;

    cross(cp,e0,e1);
    if ( fabs(cp.x) > nv_eps)
    {
        basis.a02 = -cp.y / cp.x;        
        basis.a12 = -cp.z / cp.x;
    }

    // tangent...
    T oonorm = nv_one / sqrtf(basis.a00 * basis.a00 + basis.a01 * basis.a01 + basis.a02 * basis.a02);
    basis.a00 *= oonorm;
    basis.a01 *= oonorm;
    basis.a02 *= oonorm;

    // binormal...
    oonorm = nv_one / sqrtf(basis.a10 * basis.a10 + basis.a11 * basis.a11 + basis.a12 * basis.a12);
    basis.a10 *= oonorm;
    basis.a11 *= oonorm;
    basis.a12 *= oonorm;

    // normal...
    // compute the cross product TxB
    basis.a20 = basis.a01*basis.a12 - basis.a02*basis.a11;
    basis.a21 = basis.a02*basis.a10 - basis.a00*basis.a12;
    basis.a22 = basis.a00*basis.a11 - basis.a01*basis.a10;

    oonorm = nv_one / sqrtf(basis.a20 * basis.a20 + basis.a21 * basis.a21 + basis.a22 * basis.a22);
    basis.a20 *= oonorm;
    basis.a21 *= oonorm;
    basis.a22 *= oonorm;

    // Gram-Schmidt orthogonalization process for B
    // compute the cross product B=NxT to obtain 
    // an orthogonal basis
    basis.a10 = basis.a21*basis.a02 - basis.a22*basis.a01;
    basis.a11 = basis.a22*basis.a00 - basis.a20*basis.a02;
    basis.a12 = basis.a20*basis.a01 - basis.a21*basis.a00;

    if (basis.a20 * n.x + basis.a21 * n.y + basis.a22 * n.z < nv_zero)
    {
        basis.a20 = -basis.a20;
        basis.a21 = -basis.a21;
        basis.a22 = -basis.a22;
    }
    return basis;
}

/*
 * Project an x,y pair onto a sphere of radius r OR a hyperbolic sheet
 * if we are away from the center of the sphere.
 */
 template<class T>
T tb_project_to_sphere(T r, T x, T y)
{
    T d, t, z;

    d = sqrtf(x*x + y*y);
    if (d < r * 0.70710678118654752440) {    /* Inside sphere */
        z = sqrtf(r*r - d*d);
    } else {           /* On hyperbola */
        t = r / (T)1.41421356237309504880;
        z = t*t / d;
    }
    return z;
}

/*
 * Ok, simulate a track-ball.  Project the points onto the virtual
 * trackball, then figure out the axis of rotation, which is the cross
 * product of P1 P2 and O P1 (O is the center of the ball, 0,0,0)
 * Note:  This is a deformed trackball-- is a trackball in the center,
 * but is deformed into a hyperbolic sheet of rotation away from the
 * center.  This particular function was chosen after trying out
 * several variations.
 *
 * It is assumed that the arguments to this routine are in the range
 * (-1.0 ... 1.0)
 */
template<class T>
quaternion<T> & trackball(quaternion<T>& q, vector2<T>& pt1, vector2<T>& pt2, T trackballsize)
{
    vector3<T> a; // Axis of rotation
    float phi;  // how much to rotate about axis
    vector3<T> d;
    float t;

    if (pt1.x == pt2.x && pt1.y == pt2.y) 
    {
        // Zero rotation
        q = quat_id;
        return q;
    }

    // First, figure out z-coordinates for projection of P1 and P2 to
    // deformed sphere
    vector3<T> p1(pt1.x,pt1.y,tb_project_to_sphere(trackballsize,pt1.x,pt1.y));
    vector3<T> p2(pt2.x,pt2.y,tb_project_to_sphere(trackballsize,pt2.x,pt2.y));

    //  Now, we want the cross product of P1 and P2
    cross(a,p1,p2);

    //  Figure out how much to rotate around that axis.
    d.x = p1.x - p2.x;
    d.y = p1.y - p2.y;
    d.z = p1.z - p2.z;
    t = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z) / (trackballsize);

    // Avoid problems with out-of-control values...

    if (t > nv_one)
        t = nv_one;
    if (t < -nv_one) 
        t = -nv_one;
    phi = nv_two * T(asin(t));
    axis_to_quat(q,a,phi);
    return q;
}

template<class T>
vector3<T>& cube_map_normal(int i, int x, int y, int cubesize, vector3<T>& v)
{
    T s, t, sc, tc;
    s = (T(x) + nv_one_half) / T(cubesize);
    t = (T(y) + nv_one_half) / T(cubesize);
    sc = s * nv_two - nv_one;
    tc = t * nv_two - nv_one;

    switch (i) 
    {
        case 0:
            v.x = nv_one;
            v.y = -tc;
            v.z = -sc;
            break;
        case 1:
            v.x = -nv_one;
            v.y = -tc;
            v.z = sc;
            break;
        case 2:
            v.x = sc;
            v.y = nv_one;
            v.z = tc;
            break;
        case 3:
            v.x = sc;
            v.y = -nv_one;
            v.z = -tc;
            break;
        case 4:
            v.x = sc;
            v.y = -tc;
            v.z = nv_one;
            break;
        case 5:
            v.x = -sc;
            v.y = -tc;
            v.z = -nv_one;
            break;
    }
    normalize(v);
    return v;
}

// computes the area of a triangle
template<class T>
T nv_area(const vector3<T>& v1, const vector3<T>& v2, const vector3<T>& v3)
{
    vector3<T> cp_sum;
    vector3<T> cp;
    cross(cp_sum, v1, v2);
    cp_sum += cross(cp, v2, v3);
    cp_sum += cross(cp, v3, v1);
    return nv_norm(cp_sum) * nv_one_half; 
}

// computes the perimeter of a triangle
template<class T>
T nv_perimeter(const vector3<T>& v1, const vector3<T>& v2, const vector3<T>& v3)
{
    T perim;
    vector3<T> diff;
    sub(diff, v1, v2);
    perim = nv_norm(diff);
    sub(diff, v2, v3);
    perim += nv_norm(diff);
    sub(diff, v3, v1);
    perim += nv_norm(diff);
    return perim;
}

// compute the center and radius of the inscribed circle defined by the three vertices
template<class T>
T nv_find_in_circle(vector3<T>& center, const vector3<T>& v1, const vector3<T>& v2, const vector3<T>& v3)
{
    T area = nv_area(v1, v2, v3);
    // if the area is null
    if (area < nv_eps)
    {
        center = v1;
        return nv_zero;
    }

    T oo_perim = nv_one / nv_perimeter(v1, v2, v3);

    vector3<T> diff;

    sub(diff, v2, v3);
    mult<T>(center, v1, nv_norm(diff));

    sub(diff, v3, v1);
    madd(center, v2, nv_norm(diff));
    
    sub(diff, v1, v2);
    madd(center, v3, nv_norm(diff));

    center *= oo_perim;

    return nv_two * area * oo_perim;
}

// compute the center and radius of the circumscribed circle defined by the three vertices
// i.e. the osculating circle of the three vertices
template<class T>
T nv_find_circ_circle( vector3<T>& center, const vector3<T>& v1, const vector3<T>& v2, const vector3<T>& v3)
{
    vector3<T> e0;
    vector3<T> e1;
    T d1, d2, d3;
    T c1, c2, c3, oo_c;

    sub(e0, v3, v1);
    sub(e1, v2, v1);
    dot(d1, e0, e1);

    sub(e0, v3, v2);
    sub(e1, v1, v2);
    dot(d2, e0, e1);

    sub(e0, v1, v3);
    sub(e1, v2, v3);
    dot(d3, e0, e1);

    c1 = d2 * d3;
    c2 = d3 * d1;
    c3 = d1 * d2;
    oo_c = nv_one / (c1 + c2 + c3);

    mult<T>(center,v1,c2 + c3);
    madd(center,v2,c3 + c1);
    madd(center,v3,c1 + c2);
    center *= oo_c * nv_one_half;
 
    return nv_one_half * sqrtf((d1 + d2) * (d2 + d3) * (d3 + d1) * oo_c);
}

template<class T>
T ffast_cos(const T x)
{
    // assert:  0 <= fT <= PI/2
    // maximum absolute error = 1.1880e-03
    // speedup = 2.14

    T x_sqr = x*x;
    T res = T(3.705e-02);
    res *= x_sqr;
    res -= T(4.967e-01);
    res *= x_sqr;
    res += nv_one;
    return res;
}


template<class T>
 T fast_cos(const T x)
{
    // assert:  0 <= fT <= PI/2
    // maximum absolute error = 2.3082e-09
    // speedup = 1.47

    T x_sqr = x*x;
    T res = T(-2.605e-07);
    res *= x_sqr;
    res += T(2.47609e-05);
    res *= x_sqr;
    res -= T(1.3888397e-03);
    res *= x_sqr;
    res += T(4.16666418e-02);
    res *= x_sqr;
    res -= T(4.999999963e-01);
    res *= x_sqr;
    res += nv_one;
    return res;
}

template<class T>
void nv_is_valid(const vector3<T>& v)
{
#ifdef WIN32
    assert(!_isnan(v.x) && !_isnan(v.y) && !_isnan(v.z) &&
        _finite(v.x) && _finite(v.y) && _finite(v.z));
#else
    assert(!isnan(v.x) && !isnan(v.y) && !isnan(v.z));
#endif
}

template<class T>
void nv_is_valid(T lambda)
{
#ifndef WIN32

    assert(!_isnan(lambda));
#else
    assert(!_isnan(lambda) && _finite(lambda));
#endif
}

//TL
template<class T>
T getAngle(const vector3<T> & v1, const vector3<T> & v2)
{
    float dp = dot(v1, v2);
    if(dp > 1.0f) dp = 1.0f;
    else if(dp < -1.0f) dp = -1.0f;
    return acosf(dp);
}

template<class T>
vector3<T> & rotateBy(vector3<T> & dst, const vector3<T> & src, const quaternion<T>& q)
{
    matrix3<T> M;
    quat_2_mat(M, q );
    mult<T>(dst, M, src);
    return dst;
}

// TL
template<class T>
void quaternion<T>::to_euler_xyz(vector3<T> & r)
{
    double a = 2.0f * (w*x + y*z);
    double b = 1.0 - 2.0f * (x*x + y*y);
    r.x = (T)atan2(a,b);

    a = 2.0 * (w*y - z*x);
    r.y = (T)asin(a);

    a = 2.0 * (w*z + x*y);
    b = 1.0 - 2.0 * (y*y + z*z);
    r.z = (T)atan2(a,b);
}

template<class T>
void quaternion<T>::to_euler_xyz(T* r)
{
    double a = 2.0f * (w*x + y*z);
    double b = 1.0 - 2.0f * (x*x + y*y);
    r[0] = (T)atan2(a,b);

    a = 2.0 * (w*y - z*x);
    r[1] = (T)asin(a);

    a = 2.0 * (w*z + x*y);
    b = 1.0 - 2.0 * (y*y + z*z);
    r[2] = (T)atan2(a,b);
}


template<class T>
quaternion<T>::quaternion(const vector3<T>& eulerXYZ)
{
    from_euler_xyz(eulerXYZ);
}

template<class T>
void quaternion<T>::from_euler_xyz(vector3<T> r)
{
    r *= 0.5f;
    w = cosf(r.x) * cosf(r.y)*cosf(r.z) + sinf(r.x) * sinf(r.y)*sinf(r.z);
    x = sinf(r.x) * cosf(r.y)*cosf(r.z) - cosf(r.x) * sinf(r.y)*sinf(r.z);
    y = cosf(r.x) * sinf(r.y)*cosf(r.z) + sinf(r.x) * cosf(r.y)*sinf(r.z);
    z = cosf(r.x) * cosf(r.y)*sinf(r.z) - sinf(r.x) * sinf(r.y)*cosf(r.z);
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

#if 1
template matrix3<nv_scalar>::matrix3();
template matrix3<nv_scalar>::matrix3(const nv_scalar* array);
template matrix3<nv_scalar>::matrix3(const matrix3<nv_scalar> & M);
template matrix4<nv_scalar>::matrix4();
template matrix4<nv_scalar>::matrix4(const nv_scalar* array);
template matrix4<nv_scalar>::matrix4(const matrix4<nv_scalar>& M);
template matrix4<nv_scalar>::matrix4(const matrix3<nv_scalar>& M);
template vector3<nv_scalar> & cross(vector3<nv_scalar> & u, const vector3<nv_scalar> & v, const vector3<nv_scalar> & w);
template nv_scalar & dot(nv_scalar& u, const vector2<nv_scalar>& v, const vector2<nv_scalar>& w);
template nv_scalar dot(const vector2<nv_scalar>& v, const vector2<nv_scalar>& w);
template nv_scalar & dot(nv_scalar& u, const vector3<nv_scalar>& v, const vector3<nv_scalar>& w);
template nv_scalar dot(const vector3<nv_scalar>& v, const vector3<nv_scalar>& w);
template nv_scalar & dot(nv_scalar& u, const vector4<nv_scalar>& v, const vector4<nv_scalar>& w);
template nv_scalar dot(const vector4<nv_scalar>& v, const vector4<nv_scalar>& w);
template nv_scalar & dot(nv_scalar& u, const vector3<nv_scalar>& v, const vector4<nv_scalar>& w);
template nv_scalar dot(const vector3<nv_scalar>& v, const vector4<nv_scalar>& w);
template nv_scalar & dot(nv_scalar& u, const vector4<nv_scalar>& v, const vector3<nv_scalar>& w);
template nv_scalar dot(const vector4<nv_scalar>& v, const vector3<nv_scalar>& w);
template vector3<nv_scalar> & reflect(vector3<nv_scalar>& r, const vector3<nv_scalar>& n, const vector3<nv_scalar>& l);
template vector3<nv_scalar> & madd(vector3<nv_scalar> & u, const vector3<nv_scalar>& v, const nv_scalar& lambda);
template vector3<nv_scalar> & mult(vector3<nv_scalar> & u, const vector3<nv_scalar>& v, const nv_scalar& lambda);
template vector3<nv_scalar> & mult(vector3<nv_scalar> & u, const vector3<nv_scalar>& v, const vector3<nv_scalar>& w);
template vector3<nv_scalar> & sub(vector3<nv_scalar> & u, const vector3<nv_scalar>& v, const vector3<nv_scalar>& w);
template vector3<nv_scalar> & add(vector3<nv_scalar> & u, const vector3<nv_scalar>& v, const vector3<nv_scalar>& w);
template void vector3<nv_scalar>::orthogonalize( const vector3<nv_scalar>& v );
template vector3<nv_scalar> & vector3<nv_scalar>::rotateBy(const quaternion<nv_scalar>& q);
template nv_scalar vector3<nv_scalar>::normalize();
template vector2<nv_scalar> & scale(vector2<nv_scalar>& u, const nv_scalar s);
template vector3<nv_scalar> & scale(vector3<nv_scalar>& u, const nv_scalar s);
template vector4<nv_scalar> & scale(vector4<nv_scalar>& u, const nv_scalar s);
template vector3<nv_scalar> & mult(vector3<nv_scalar>& u, const matrix3<nv_scalar>& M, const vector3<nv_scalar>& v);
template vector3<nv_scalar> & mult(vector3<nv_scalar>& u, const vector3<nv_scalar>& v, const matrix3<nv_scalar>& M);
template const vector3<nv_scalar> operator*(const matrix3<nv_scalar>& M, const vector3<nv_scalar>& v);
template const vector3<nv_scalar> operator*(const vector3<nv_scalar>& v, const matrix3<nv_scalar>& M);
template vector4<nv_scalar> & mult(vector4<nv_scalar>& u, const matrix4<nv_scalar>& M, const vector4<nv_scalar>& v);
template vector4<nv_scalar> & mult(vector4<nv_scalar>& u, const vector4<nv_scalar>& v, const matrix4<nv_scalar>& M);
template const vector4<nv_scalar> operator*(const matrix4<nv_scalar>& M, const vector4<nv_scalar>& v);
template const vector4<nv_scalar> operator*(const vector4<nv_scalar>& v, const matrix4<nv_scalar>& M);
template vector3<nv_scalar> & mult_pos(vector3<nv_scalar>& u, const matrix4<nv_scalar>& M, const vector3<nv_scalar>& v);
template vector3<nv_scalar> & mult_pos(vector3<nv_scalar>& u, const vector3<nv_scalar>& v, const matrix4<nv_scalar>& M);
template vector3<nv_scalar> & mult_dir(vector3<nv_scalar>& u, const matrix4<nv_scalar>& M, const vector3<nv_scalar>& v);
template vector3<nv_scalar> & mult_dir(vector3<nv_scalar>& u, const vector3<nv_scalar>& v, const matrix4<nv_scalar>& M);
template vector3<nv_scalar> & mult(vector3<nv_scalar>& u, const matrix4<nv_scalar>& M, const vector3<nv_scalar>& v);
template vector3<nv_scalar> & mult(vector3<nv_scalar>& u, const vector3<nv_scalar>& v, const matrix4<nv_scalar>& M);
template matrix4<nv_scalar> & add(matrix4<nv_scalar>& A, const matrix4<nv_scalar>& B);
template matrix3<nv_scalar> & add(matrix3<nv_scalar>& A, const matrix3<nv_scalar>& B);
template matrix4<nv_scalar> & add(matrix4<nv_scalar> & C, const matrix4<nv_scalar> & A, const matrix4<nv_scalar> & B);
template matrix3<nv_scalar> & add(matrix3<nv_scalar> & C, const matrix3<nv_scalar> & A, const matrix3<nv_scalar> & B);
template matrix4<nv_scalar> matrix4<nv_scalar>::operator*(const matrix4<nv_scalar>& B) const;
template matrix4<nv_scalar> & mult(matrix4<nv_scalar>& C, const matrix4<nv_scalar>& A, const matrix4<nv_scalar>& B);
template matrix3<nv_scalar> & mult(matrix3<nv_scalar>& C, const matrix3<nv_scalar>& A, const matrix3<nv_scalar>& B);
template matrix3<nv_scalar> & transpose(matrix3<nv_scalar>& A);
template matrix4<nv_scalar> & transpose(matrix4<nv_scalar>& A);
template matrix4<nv_scalar> & transpose(matrix4<nv_scalar>& B, const matrix4<nv_scalar>& A);
template matrix3<nv_scalar> & transpose(matrix3<nv_scalar>& B, const matrix3<nv_scalar>& A);
template nv_scalar det2x2(nv_scalar a1, nv_scalar a2, nv_scalar b1, nv_scalar b2);
template nv_scalar det3x3(nv_scalar a1, nv_scalar a2, nv_scalar a3, nv_scalar b1, nv_scalar b2, nv_scalar b3, nv_scalar c1, nv_scalar c2, nv_scalar c3);
template matrix4<nv_scalar> & invert(matrix4<nv_scalar>& B, const matrix4<nv_scalar>& A);
template matrix4<nv_scalar> & invert_rot_trans(matrix4<nv_scalar>& B, const matrix4<nv_scalar>& A);
template nv_scalar det(const matrix3<nv_scalar>& A);
template matrix3<nv_scalar> & invert(matrix3<nv_scalar>& B, const matrix3<nv_scalar>& A);
template vector2<nv_scalar> & normalize(vector2<nv_scalar>& u);
template vector3<nv_scalar> & normalize(vector3<nv_scalar>& u);
template vector4<nv_scalar> & normalize(vector4<nv_scalar>& u);
template quaternion<nv_scalar> & normalize(quaternion<nv_scalar> & p);
template matrix4<nv_scalar> & look_at(matrix4<nv_scalar>& M, const vector3<nv_scalar>& eye, const vector3<nv_scalar>& center, const vector3<nv_scalar>& up);
template matrix4<nv_scalar> & frustum(matrix4<nv_scalar>& M, const nv_scalar l, const nv_scalar r, const nv_scalar b, const nv_scalar t, const nv_scalar n, const nv_scalar f);
template matrix4<nv_scalar> & perspective(matrix4<nv_scalar>& M, const nv_scalar fovy, const nv_scalar aspect, const nv_scalar n, const nv_scalar f);
template matrix4<nv_scalar> & ortho(matrix4<nv_scalar> & M, const nv_scalar left, 
                              const nv_scalar right, 
                              const nv_scalar bottom, 
                              const nv_scalar top,
                              const nv_scalar n,
                              const nv_scalar f);
template quaternion<nv_scalar>::quaternion();
template quaternion<nv_scalar>::quaternion(nv_scalar *q);
template quaternion<nv_scalar>::quaternion(nv_scalar x, nv_scalar y, nv_scalar z, nv_scalar w);
template quaternion<nv_scalar>::quaternion(const quaternion<nv_scalar>& quaternion);
template quaternion<nv_scalar>::quaternion(const vector3<nv_scalar>& axis, nv_scalar angle);
template quaternion<nv_scalar>::quaternion(const matrix3<nv_scalar>& rot);
template quaternion<nv_scalar>::quaternion(const matrix4<nv_scalar>& rot);
template quaternion<nv_scalar>& quaternion<nv_scalar>::operator=(const quaternion<nv_scalar>& quaternion);
template quaternion<nv_scalar> quaternion<nv_scalar>::inverse();
template quaternion<nv_scalar> quaternion<nv_scalar>::conjugate();
template void quaternion<nv_scalar>::normalize();
template void quaternion<nv_scalar>::from_matrix(const matrix3<nv_scalar>& mat);
template void quaternion<nv_scalar>::from_matrix(const matrix4<nv_scalar>& mat);
template void quaternion<nv_scalar>::to_matrix(matrix3<nv_scalar>& mat) const;
template void quaternion<nv_scalar>::to_matrix(matrix4<nv_scalar>& mat) const;
template const quaternion<nv_scalar> operator*(const quaternion<nv_scalar>& p, const quaternion<nv_scalar>& q);
template quaternion<nv_scalar>& quaternion<nv_scalar>::operator*=(const quaternion<nv_scalar>& q);
template matrix3<nv_scalar> & quat_2_mat(matrix3<nv_scalar>& M, const quaternion<nv_scalar>& q);
template quaternion<nv_scalar> & mat_2_quat(quaternion<nv_scalar>& q, const matrix3<nv_scalar>& M);
template quaternion<nv_scalar> & mat_2_quat(quaternion<nv_scalar>& q, const matrix4<nv_scalar>& M);
template quaternion<nv_scalar> & axis_to_quat(quaternion<nv_scalar>& q, const vector3<nv_scalar>& a, const nv_scalar phi);
template quaternion<nv_scalar> & conj(quaternion<nv_scalar> & p);
template quaternion<nv_scalar> & conj(quaternion<nv_scalar>& p, const quaternion<nv_scalar>& q);
template quaternion<nv_scalar> & add_quats(quaternion<nv_scalar>& p, const quaternion<nv_scalar>& q1, const quaternion<nv_scalar>& q2);
template nv_scalar & dot(nv_scalar& s, const quaternion<nv_scalar>& q1, const quaternion<nv_scalar>& q2);
template nv_scalar dot(const quaternion<nv_scalar>& q1, const quaternion<nv_scalar>& q2);
template quaternion<nv_scalar> & slerp_quats(quaternion<nv_scalar> & p, nv_scalar s, const quaternion<nv_scalar> & q1, const quaternion<nv_scalar> & q2);
template nv_scalar nv_random();
template matrix3<nv_scalar> & matrix3<nv_scalar>::set_rot(const nv_scalar& theta, const vector3<nv_scalar>& v) ;
template matrix3<nv_scalar> & matrix3<nv_scalar>::set_rot(const vector3<nv_scalar>& u, const vector3<nv_scalar>& v);
template nv_scalar matrix3<nv_scalar>::norm_one();
template nv_scalar matrix3<nv_scalar>::norm_inf();
template matrix4<nv_scalar> & matrix4<nv_scalar>::set_rot(const quaternion<nv_scalar>& q);
template matrix4<nv_scalar> & matrix4<nv_scalar>::set_rot(const nv_scalar& theta, const vector3<nv_scalar>& v) ;
template matrix4<nv_scalar> & matrix4<nv_scalar>::set_rot(const vector3<nv_scalar>& u, const vector3<nv_scalar>& v);
template matrix4<nv_scalar> & matrix4<nv_scalar>::set_rot(const matrix3<nv_scalar>& M);
template matrix4<nv_scalar> & matrix4<nv_scalar>::set_scale(const vector3<nv_scalar>& s);
template vector3<nv_scalar>& matrix4<nv_scalar>::get_scale(vector3<nv_scalar>& s) const;
template matrix4<nv_scalar> & matrix4<nv_scalar>::as_scale(const vector3<nv_scalar>& s);
template matrix4<nv_scalar> & matrix4<nv_scalar>::as_scale(const nv_scalar& s);
template matrix4<nv_scalar> & matrix4<nv_scalar>::set_translation(const vector3<nv_scalar>& t);
template matrix4<nv_scalar> & matrix4<nv_scalar>::set_translate(const vector3<nv_scalar>& t);
template matrix4<nv_scalar> & matrix4<nv_scalar>::as_translation(const vector3<nv_scalar>& t);
template vector3<nv_scalar> & matrix4<nv_scalar>::get_translation(vector3<nv_scalar>& t) const;
template matrix3<nv_scalar> & matrix4<nv_scalar>::get_rot(matrix3<nv_scalar>& M) const;
template quaternion<nv_scalar> & matrix4<nv_scalar>::get_rot(quaternion<nv_scalar>& q) const;
template matrix4<nv_scalar> & negate(matrix4<nv_scalar> & M);
template matrix3<nv_scalar> & negate(matrix3<nv_scalar> & M);
template matrix3<nv_scalar>& tangent_basis(matrix3<nv_scalar>& basis, const vector3<nv_scalar>& v0, const vector3<nv_scalar>& v1, const vector3<nv_scalar>& v2, const vector2<nv_scalar>& t0, const vector2<nv_scalar>& t1, const vector2<nv_scalar>& t2, const vector3<nv_scalar> & n);
template nv_scalar tb_project_to_sphere(nv_scalar r, nv_scalar x, nv_scalar y);
template quaternion<nv_scalar> & trackball(quaternion<nv_scalar>& q, vector2<nv_scalar>& pt1, vector2<nv_scalar>& pt2, nv_scalar trackballsize);
template vector3<nv_scalar>& cube_map_normal(int i, int x, int y, int cubesize, vector3<nv_scalar>& v);
template nv_scalar nv_area(const vector3<nv_scalar>& v1, const vector3<nv_scalar>& v2, const vector3<nv_scalar>& v3);
template nv_scalar nv_perimeter(const vector3<nv_scalar>& v1, const vector3<nv_scalar>& v2, const vector3<nv_scalar>& v3);
template nv_scalar nv_find_in_circle(vector3<nv_scalar>& center, const vector3<nv_scalar>& v1, const vector3<nv_scalar>& v2, const vector3<nv_scalar>& v3);
template nv_scalar nv_find_circ_circle( vector3<nv_scalar>& center, const vector3<nv_scalar>& v1, const vector3<nv_scalar>& v2, const vector3<nv_scalar>& v3);
template nv_scalar ffast_cos(const nv_scalar x);
template nv_scalar fast_cos(const nv_scalar x);
template void nv_is_valid(const vector3<nv_scalar>& v);
template void nv_is_valid(nv_scalar lambda);
template nv_scalar getAngle(const vector3<nv_scalar> & v1, const vector3<nv_scalar> & v2);
template vector3<nv_scalar> & rotateBy(vector3<nv_scalar> & dst, const vector3<nv_scalar> & src, const quaternion<nv_scalar>& q);
template void quaternion<nv_scalar>::to_euler_xyz(vector3<nv_scalar> & r);
template void quaternion<nv_scalar>::to_euler_xyz(nv_scalar* r);
template quaternion<nv_scalar>::quaternion(const vector3<nv_scalar>& eulerXYZ);
template void quaternion<nv_scalar>::from_euler_xyz(vector3<nv_scalar> r);

} //namespace nv_math

#endif
