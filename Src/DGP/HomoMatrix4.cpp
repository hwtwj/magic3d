//#include "StdAfx.h"
#include "HomoMatrix4.h"
#include "Eigen/Geometry"
#include "../Common/LogSystem.h"

namespace MagicDGP
{
    HomoMatrix4::HomoMatrix4()
    {
        Unit();
    }

    HomoMatrix4::HomoMatrix4(const HomoMatrix4& mat)
    {
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                mValues[i * 4 + j] = mat.GetValue(i, j);
            }
        }
    }

    HomoMatrix4::HomoMatrix4(const Real* pValues)
    {
        for (int i = 0; i < 16; i++)
        {
            mValues[i] = pValues[i];
        }
    }

    HomoMatrix4::~HomoMatrix4()
    {
    }

    void HomoMatrix4::Print() const
    {
        DebugLog << "Matrix: " << std::endl; 
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                DebugLog << mValues[4 * i + j] << " ";
            }
            DebugLog << std::endl;
        }
    }

    Vector3 HomoMatrix4::TransformPoint(const Vector3& vec) const
    {
        Vector3 vecRes;
        vecRes[0] = mValues[0] * vec[0] + mValues[1] * vec[1] + mValues[2] * vec[2] + mValues[3];
        vecRes[1] = mValues[4] * vec[0] + mValues[5] * vec[1] + mValues[6] * vec[2] + mValues[7];
        vecRes[2] = mValues[8] * vec[0] + mValues[9] * vec[1] + mValues[10] * vec[2] + mValues[11];
        
        return vecRes;
    }

    Vector3 HomoMatrix4::RotateVector(const Vector3& vec) const
    {
        Vector3 vecRes;
        vecRes[0] = mValues[0] * vec[0] + mValues[1] * vec[1] + mValues[2] * vec[2];
        vecRes[1] = mValues[4] * vec[0] + mValues[5] * vec[1] + mValues[6] * vec[2];
        vecRes[2] = mValues[8] * vec[0] + mValues[9] * vec[1] + mValues[10] * vec[2];

        return vecRes;
    }

    HomoMatrix4 HomoMatrix4::operator * (const HomoMatrix4& mat) const
    {
        HomoMatrix4 matRes;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                Real vTemp = 0;
                for (int k = 0; k < 4; k++)
                {
                    vTemp += GetValue(i, k) * mat.GetValue(k, j);
                }
                matRes.SetValue(i, j, vTemp);
            }
        }

        return matRes;
    }

    HomoMatrix4 HomoMatrix4::operator *=(const HomoMatrix4& mat)
    {
        HomoMatrix4 matCopy = *this;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                Real vTemp = 0;
                for (int k = 0; k < 4; k++)
                {
                    vTemp += matCopy.GetValue(i, k) * mat.GetValue(k, j);
                }
                SetValue(i, j, vTemp);
            }
        }
        return *this;
    }

    Real HomoMatrix4::GetValue(int row, int col) const
    {
        return mValues[row * 4 + col];
    }

    void HomoMatrix4::SetValue(int row, int col, Real v)
    {
        mValues[row * 4 + col] = v;
    }

    void HomoMatrix4::Unit()
    {
        for (int i = 0; i < 16; i++)
        {
            mValues[i] = 0;
        }
        mValues[0] = 1;
        //mValues[3] = 1;
        mValues[5] = 1;
        //mValues[7] = 1;
        mValues[10] = 1;
        //mValues[11] = 1;
        mValues[15] = 1;
    }

    HomoMatrix4 HomoMatrix4::Inverse()
    {
        Eigen::Transform<double, 3, Eigen::Affine> transform;
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                transform(i, j) = GetValue(i, j);
            }
        }
        Eigen::Transform<double, 3, Eigen::Affine> transInv = transform.inverse();
        HomoMatrix4 inv;
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                inv.SetValue(i, j, transInv(i, j));
            }
        }
        return inv;
    }

    void HomoMatrix4::GenerateVectorToVectorRotation(const Vector3& source, const Vector3& target)
    {
        Vector3 v = source.CrossProduct(target);
        Real vLengthSquared = v.LengthSquared();
        if (vLengthSquared < Epsilon)
        {
            Unit();
        }
        else
        {
            Real e = source * target;
            Real h = (1.0 - e) / (v * v);
            mValues[0] = e + h * v[0] * v[0];
            mValues[1] = h * v[0] * v[1] - v[2];
            mValues[2] = h * v[0] * v[2] + v[1];
            mValues[3] = 0;
            mValues[4] = h * v[0] * v[1] + v[2];
            mValues[5] = e + h * v[1] * v[1];
            mValues[6] = h * v[1] * v[2] - v[0];
            mValues[7] = 0;
            mValues[8] = h * v[0] * v[2] - v[1];
            mValues[9] = h * v[1] * v[2] + v[0];
            mValues[10] = e + h * v[2] * v[2];
            mValues[11] = 0;
            mValues[12] = 0;
            mValues[13] = 0;
            mValues[14] = 0;
            mValues[15] = 1;
        }
    }

    void HomoMatrix4::GenerateTranslation(const Vector3& translate)
    {
        mValues[0] = 1;
        mValues[1] = 0;
        mValues[2] = 0;
        mValues[3] = translate[0];
        mValues[4] = 0;
        mValues[5] = 1;
        mValues[6] = 0;
        mValues[7] = translate[1];
        mValues[8] = 0;
        mValues[9] = 0;
        mValues[10] = 1;
        mValues[11] = translate[2];
        mValues[12] = 0;
        mValues[13] = 0;
        mValues[14] = 0;
        mValues[15] = 1;
    }
}