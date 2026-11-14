#ifndef RWS_MATHS_H
#define RWS_MATHS_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (maths.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\toolkits\math\maths.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// ============================================================================

#include <rwcore.h>
#include <cmath>
#include <cassert>

namespace RWS
{
    /**
     * @namespace RWS::Maths
     * @brief Math, vector operations and trigonometry helpers.
     *
     * Contains the core linear algebra operations like rotation calculations, interpolation (Lerp)
     * and vector products inside the game engine.
     * Ship physics, AI (PathFinding) and projectile trajectories rely
     * on these utilities.
     */
    namespace Maths
    {
        /// Pi constant (3.1415...)
        const RwReal PI = 3.14159265358979323846f;
        /// 2 * Pi (360 derece)
        const RwReal TWO_PI = 6.28318530717958647692f;
        /// Pi / 2 (90 derece)
        const RwReal HALF_PI = 1.57079632679489661923f;
        /// Degree to radian conversion factor (Pi / 180)
        const RwReal DEG_TO_RAD = 0.01745329251994329576f;
        /// Radian to degree conversion factor (180 / Pi)
        const RwReal RAD_TO_DEG = 57.2957795130823208767f;
        /// Floating-point (float) error tolerance / near-zero limit
        const RwReal EPSILON = 0.00001f;

        /**
         * @brief Converts an angle in degrees to radians.
         * @param deg Derece (0-360)
         * @return RwReal — Radyan (0 - 2PI)
         */
        inline RwReal DegToRad(RwReal deg) { return deg * DEG_TO_RAD; }

        /**
         * @brief Converts an angle in radians to degrees.
         * @param rad Radyan (0 - 2PI)
         * @return RwReal — Derece (0-360)
         */
        inline RwReal RadToDeg(RwReal rad) { return rad * RAD_TO_DEG; }

        /**
         * @brief Clamps the value into the given minimum and maximum range (Clamp).
         * @param val    Value to test.
         * @param minVal Minimum bound.
         * @param maxVal Maximum bound.
         * @return T — Value clamped within the bounds.
         */
        template <typename T>
        inline T Clamp(T val, T minVal, T maxVal)
        {
            assert(maxVal >= minVal && "Failed PRE-condition: max >= min");
            if (val < minVal) return minVal;
            if (val > maxVal) return maxVal;
            return val;
        }

        /**
         * @brief Returns the smaller of two values.
         */
        template <typename T>
        inline T Min(T a, T b) { return (a < b) ? a : b; }

        /**
         * @brief Returns the larger of two values.
         */
        template <typename T>
        inline T Max(T a, T b) { return (a > b) ? a : b; }

        /**
         * @brief Linearly interpolates between two floating-point numbers (Lerp).
         * @param a Start value.
         * @param b End value.
         * @param t Blend weight (0.0 - 1.0).
         * @return RwReal — The interpolated value.
         */
        inline RwReal Lerp(RwReal a, RwReal b, RwReal t)
        {
            return a + (b - a) * t;
        }

        /**
         * @brief Linearly interpolates between two vectors.
         * @param pOut Output vector (computed value is written here).
         * @param pA   Start vector.
         * @param pB   End vector.
         * @param t    Blend weight (0.0 - 1.0).
         */
        inline void VectorLerp(RwV3d* pOut, const RwV3d* pA, const RwV3d* pB, RwReal t)
        {
            pOut->x = Lerp(pA->x, pB->x, t);
            pOut->y = Lerp(pA->y, pB->y, t);
            pOut->z = Lerp(pA->z, pB->z, t);
        }

        /**
         * @brief Returns the vector's magnitude (length).
         * @param pV Target vector.
         * @return RwReal — Uzunluk.
         */
        inline RwReal VectorLength(const RwV3d* pV)
        {
            return std::sqrt(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
        }

        /**
         * @brief Returns the vector's squared length (without square root).
         *
         * Faster when only distance comparisons are needed.
         */
        inline RwReal VectorLengthSquared(const RwV3d* pV)
        {
            return pV->x * pV->x + pV->y * pV->y + pV->z * pV->z;
        }

        /**
         * @brief Converts the vector to a unit vector (normalizes it) and returns the old length.
         * @param pV Vector to normalize.
         * @return RwReal — The vector's length before normalization.
         */
        inline RwReal VectorNormalize(RwV3d* pV)
        {
            RwReal len = VectorLength(pV);
            if (len > EPSILON)
            {
                RwReal invLen = 1.0f / len;
                pV->x *= invLen;
                pV->y *= invLen;
                pV->z *= invLen;
            }
            return len;
        }

        /**
         * @brief Computes the scalar (dot / Dot) product of two vectors.
         * @param pA First vector.
         * @param pB Second vector.
         * @return RwReal — Result of the scalar product (cosine similarity).
         */
        inline RwReal VectorDotProduct(const RwV3d* pA, const RwV3d* pB)
        {
            return pA->x * pB->x + pA->y * pB->y + pA->z * pB->z;
        }

        /**
         * @brief Computes the vector (cross / Cross) product of two vectors.
         * @param pOut Output vector (new vector perpendicular to both vectors).
         * @param pA   First vector.
         * @param pB   Second vector.
         */
        inline void VectorCrossProduct(RwV3d* pOut, const RwV3d* pA, const RwV3d* pB)
        {
            pOut->x = pA->y * pB->z - pA->z * pB->y;
            pOut->y = pA->z * pB->x - pA->x * pB->z;
            pOut->z = pA->x * pB->y - pA->y * pB->x;
        }
    }
}

#endif // RWS_MATHS_H

