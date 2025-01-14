#pragma once

#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include "ceres/jet.h"
namespace adekf
{

    /**
     * Common base class for all Manifolds
     * Used to detect Manifolds
     */
    struct Manifold
    {
    };
    /**
     * Common case class for CompoundManifolds
     * Used to detect CompoundManifolds
     */
    struct CompoundManifold : public Manifold
    {
    };

    /**
         * Generates a Vector of dual Components
         * @tparam Size The Size of the Vector and the dual components
         * @return A dual component vector, to be added to a state
         */
    template <unsigned Size>
    const Eigen::Matrix<ceres::Jet<double, Size>, Size, 1> &getDerivator()
    {
        //The resulting dual component vector
        static Eigen::Matrix<ceres::Jet<double, Size>, Size, 1> result;
        //only set on first call
        static bool first_call = true;
        //Set the first coefficient in the first row to 1, the second in the second and so on.
        if (first_call)
        {
            result.setZero();
            for (unsigned i = 0; i < Size; ++i)
                result[i].v[i] = 1; // = ceres::Jet<ScalarType, Size>(0, i);
            first_call = false;
        }
        return result;
    }

    /**
     * Retrieves Scalar Type and DOF from a given Manifold Class
     * @tparam T The given class
     * @tparam s whether the class is an Eigen Class
     */
    template <typename T, bool s = std::is_convertible_v<std::remove_const_t<T> *, Manifold *>>
    struct StateInfo
    {
    };
    /**
     * Specialisation of StateInfo for & parameters
     */
    template <typename amp, bool s>
    struct StateInfo<amp &, s> : StateInfo<amp, s>
    {
    };

    /**
     * Specialisation of StateInfo for Manifolds
     */
    template <typename T>
    struct StateInfo<T, true>
    {
        using ScalarType = typename T::ScalarType;
        static constexpr int DOF = T::DOF;
#ifdef MANIFOLD_WITH_CERES
        static constexpr int GLOBAL_SIZE = T::GLOBAL_SIZE;
#else
        static constexpr int GLOBAL_SIZE = T::GlobalSize;
#endif
        using type = T;
    };

    /**
     * Specialization of StateInfo for Eigen types
     */
    template <typename DERIVED>
    struct StateInfo<DERIVED, false>
    {
        using ScalarType = typename Eigen::internal::traits<typename DERIVED::PlainObject>::Scalar;
        static constexpr int DOF = Eigen::internal::traits<typename DERIVED::PlainObject>::RowsAtCompileTime;
        static constexpr int GLOBAL_SIZE = DOF;
        using type = Eigen::Matrix<typename DERIVED::Scalar, DERIVED::RowsAtCompileTime, DERIVED::ColsAtCompileTime>;
    };

    /**
     * Specialization of StateInfo for float types
     */

    template <bool s>
    struct StateInfo<float, s>
    {
        using ScalarType = float;
        static constexpr int DOF = 1;
        static constexpr int GLOBAL_SIZE = DOF;
        using type = float;
    };

    /**
     * Specialisation of StateInfo for double types
     *
     */
    template <>
    struct StateInfo<double, false>
    {
        using ScalarType = double;
        static constexpr int DOF = 1;
        static constexpr int GLOBAL_SIZE = DOF;
        using type = double;
    };

    /**
     * Retreives the DOF of a Manifold or Vector class
     * @tparam T The class to be used
     */
    template <typename T>
    static constexpr int DOFOf = StateInfo<typename std::remove_reference<T>::type>::DOF;

    /**
     * Retreives the Global Size of a Manifold or Vector class
     * @tparam T The class to be used
     */
    template <typename T>
    static constexpr int GlobalOf = StateInfo<typename std::remove_reference<T>::type>::GLOBAL_SIZE;

    /**
     * Retrieves the scalar type of a state
     * @param state the variable to derive the scalar type from
     */
#define ScalarOf(state) typename adekf::StateInfo<typename std::remove_reference<decltype(state)>::type>::ScalarType
    template <typename T>
    using ScalarOf = typename adekf::StateInfo<typename std::remove_reference<T>::type>::ScalarType;

/*
 * Macro to read DOF from parameter name, can be used to read DOF of auto parameters
 */
#define ADEKF_GETDOF(name) adekf::DOFOf<decltype(name)>
/*
 * Macro to read Global Size from parameter name, can be used to read Global of auto parameters
 */
#define ADEKF_GETGLOBAL(name) adekf::GlobalOf<decltype(name)>

    template <int N, int M>
    static constexpr bool dynamicMatrix = N *M > USE_EIGEN_DYNAMIC_THRESHHOLD;

    /**
         * A Matrix type (It is the Eigen::Matrix)
         * @tparam N Number of Rows
         * @tparam M Number of Columns
         */
    template <typename ScalarType, int N, int M>
    using MatrixType = Eigen::Matrix<ScalarType, N, M>;
    /**
         * An Eigen Matrix which chooses automatically when it is usefull to use dynamic allocation
         * @tparam N Number of Rows
         * @tparam M Number of Columns
         */
    template <typename ScalarType, int N, int M>
    using AutoMatrixType = typename std::conditional<dynamicMatrix<N, M>, Eigen::Matrix<double, -1, -1>, Eigen::Matrix<double, N, N>>::type;

    /**
         * A Square Matrix with the Scalar Type of the State
         * @tparam N Number of Rows and Columns
         */
    template <typename ScalarType, int N>
    using SquareMatrixType = MatrixType<ScalarType, N, N>;

    /**
     * Covariance Matrix of a Manifold or Vector class
     * @tparam The class to be used
     */
    template <typename T>
    using CovarianceOf = SquareMatrixType<T, DOFOf<T>>;

    /**
     * A Helper Function to bypass Eigen Expression Templates
     * This is necessary to be able to get type information from expression results
     * @tparam T Type of the expression
     * @param result The Expression to be bypassed
     * @return The bypassed expression
     */
    template <typename T>
    auto eval(const T &result)
    {
        return result;
    }

    /**
     * A Helper Function to bypass Eigen Expression Templates
     * This is necessary to be able to get type information from expression results
     * @tparam BinaryOp The Type of Operation
     * @tparam LhsType Left Type
     * @tparam RhsType Right Type
     * @param result The Expression to be bypassed
     * @return The bypassed expression
     */
    template <typename BinaryOp, typename LhsType, typename RhsType>
    auto eval(const Eigen::CwiseBinaryOp<BinaryOp, LhsType, RhsType> &result)
    {
        //The eval() function returns the result of the operation
        //If this is not used auto will not work correctly
        return result.eval();
    }

    template <typename XprType, int BlockRows, int BlockCols, bool InnerPanel>
    auto eval(const Eigen::Block<XprType, BlockRows, BlockCols, InnerPanel> &result)
    {
        return result.eval();
    }

    //Macros to split noise vector into parts
/**
 * Cuts a segment from the vector noise from start to start+size
 *
 * @param noise  the noise vector
 * @param start  the start of the segment of the noise
 * @param size the size of the noise vector segment
 */
#define NOISE_WITH_OTHER_NAME(noise, start, size) noise.template segment<size>(start)
/**
 * Calls NOISE_WITH_OTHER_NAME but assumes, that the noise vector is called "noise"
 * @param start  the start of the segment of the noise
 * @param size the size of the noise vector segment
 */
#define NOISE_WITHOUT_NOISE_NAME(start, size) NOISE_WITH_OTHER_NAME(noise, start, size)
/**
 * Helper Macro to decide which noise macro to take
 */
#define GET_NOISE_MACRO(_1, _2, _3, NAME, ...) NAME
/**
 * Cuts a segment from the vector noise from start to start+size
 *
 * Calls either NOISE_WITH_OTHER_NAME or NOISE_WITHOUT_NOISE_NAME depending on the amount of args
 * call NOISE(<start>, <size>) if your noise vector is called "noise"
 * call NOISE(<noise-vector>,<start>,<size>) otherwise
 *
 * Results in a segment of the noise vector from start to start+size (exclusive) of length size
 */
#define NOISE(...) GET_NOISE_MACRO(__VA_ARGS__, NOISE_WITH_OTHER_NAME, NOISE_WITHOUT_NOISE_NAME) \
(__VA_ARGS__)

    //Macros for output operations

#ifndef LOG_STREAM
/**
 * Stream Macro defaults to std::cout
 * User can define to send data to any other stream (e.g. a file stream)
 */
#define LOG_STREAM std::cout
#endif
#ifndef ERR_STREAM
/**
 * Error stream macro defaults to std::err
 * User can define to send data to any other stream
 */
#define ERR_STREAM std::err
#endif
//just a define for << std::endl
#define LOG_END << std::endl;

//Macros for Type results
/**
 * The resulting type if you add scalars of type T1 and T2  (T1+T2)
 */
#define ADEKF_PLUSRESULT(T1, T2) decltype(std::declval<T1>() + std::declval<T2>())

/**
 * The resulting type if you subtract scalars of type T1 and T2 (T1-T2)
 */
#define ADEKF_MINUSRESULT(T1, T2) decltype(std::declval<T1>() - std::declval<T2>())

    /**
 * Determines whether all matrix elements are not inf or nan
 * @param matrix The matrix to check
 * @return	whether all values are finite
 */
    template <typename Derived>
    bool isfinite(const Eigen::MatrixBase<Derived> &matrix)
    {
        return matrix.allFinite();
    }
    /**
 * does nothing just for variadic resolve of assert_inputs(T first,ARGS) with 0 arguments
 */
    void inline assert_finite()
    {
    }
    /**
 * asserts whether all input numbers have finite values
 *
 * Uses variadic Evaluation to check an undefined amount of variables
 * @param first  the argument which is evaluated at this call
 * @param args   arguments for next calls
 */

    template <typename T, typename... Args>
    void assert_finite(T first, Args... args)
    {
        assert(isfinite(first));
        assert_finite(args...);
    }

    //Vector type to support optimiziation on Eigen and State Types on std::vector (usual std::vector breaks)
    template <class AlignedClass>
    using aligned_vector = std::vector<AlignedClass, Eigen::aligned_allocator<AlignedClass>>;

    /**
 * Checks whether  a matrix is positive definite (usefull for covariance)
 * 
 * Computationally expensive since it uses ldlt decomposition
 * @tparam Derived type of the Matrix
 * @param Matrix to Matrix to check
 * 
 * @return true if matrix is positive definite
 */
    template <class Derived>
    bool isPositiveDefinite(const Eigen::MatrixBase<Derived> &matrix)
    {
        auto _ldlt = matrix.ldlt();
        return _ldlt.info() == Eigen::Success && _ldlt.isPositive();
    }

    /**
 * Sets all eigenvalues to the minimum value eps.
 * 
 * Computationally expensive since it uses eigen decomposition
 * 
 * @tparam Derived type of the Matrix
 * @param matrix to regularize, inplace operation
 * @param eps the minimum value of the eigen values
 * 
 */
    template <class Derived>
    void assurePositiveDefinite(Eigen::MatrixBase<Derived> &matrix, double eps = 1e-10)
    {
        Eigen::SelfAdjointEigenSolver<Derived> svd{matrix};
        Eigen::VectorXd sV = svd.eigenvalues();
        bool positive = true;
        for (int i = 0; i < sV.rows(); i++)
        {
            if (sV(i) < eps)
            {
                sV(i) = eps;
                positive = false;
            }
        }

        if (positive)
            return;
        else
        {
            matrix = svd.eigenvectors() * sV.asDiagonal() * svd.eigenvectors().inverse();
            assert(isPositiveDefinite(matrix));
        }
    }
    /**
 * @brief Extracts a Jacobian from the given vector with jets.
 * 
 * @tparam ScalarType The Scalartype of the Jet
 * @tparam _LDOF The number of Rows
 * @tparam _RDOF The number of Cols
 * @param result The vector to extract the Jacobi from. Has to be the result of automatic Differentiation with jets
 * @return MatrixType<ScalarType,_LDOF,_RDOF> The extracted Jacobian. 
 */
    template <typename ScalarType, int _LDOF, int _RDOF>
    AutoMatrixType<ScalarType, _LDOF, _RDOF> extractJacobi(const Eigen::Matrix<ceres::Jet<ScalarType, _RDOF>, _LDOF, 1> &result)
    {
        AutoMatrixType<ScalarType, _LDOF, _RDOF> jacobi(_LDOF, _RDOF);
        for (size_t j = 0; j < _LDOF; ++j)
        {
            jacobi.row(j) = result(j).v;
        }
        return jacobi;
    }

    /**
     * @brief Calculates the transformation Jacobian that transform from reference r1 to reference r2
     * 
     * @tparam Manifold The type of the manifold
     * @tparam DOF The degree of freedom of the manifold
     * @param ref1 The base reference
     * @param ref2 The target reference
     * @param Er1  The expected value of the Gaussian in the base reference
     * @return MatrixType<ScalarOf<Manifold>,DOF,DOF> The jacobian that transforms the covariance from the base to target reference
     */
    template <typename ManifoldType, int DOF = DOFOf<ManifoldType>>
    inline AutoMatrixType<ScalarOf<ManifoldType>, DOF, DOF> transformReferenceJacobian(const ManifoldType &ref1, const ManifoldType &ref2, const decltype(ref2 - ref1) &Er1)
    {
        if constexpr (!std::is_base_of<CompoundManifold, ManifoldType>::value)
        {
            return extractJacobi(ref1 + (Er1 + getDerivator<DOF>()) - ref2);
        }
        else
        {
            //Definition of the Jacobian of the Boxplus Function
            AutoMatrixType<ScalarOf<ManifoldType>, DOF, DOF> D = D.Identity(DOF, DOF);
            //Counter for the current dof at iterating
            int dof = 0;
            //calculate the part of the Jacobian which belongs to the Manifold
            auto calcManifoldJacobian = [&](auto &manifold, auto & other_manifold) {
                int constexpr curDOF = DOFOf<decltype(manifold)>;
                //Calculate the Jacobian of the Boxplus Function
                auto cur_Er1 = Er1.template segment<curDOF>(dof);
                D.template block<curDOF, curDOF>(dof, dof) = transformReferenceJacobian(manifold, other_manifold, cur_Er1);
                dof += curDOF;
            };
            //apply on each manifold, for vectors the Jacobian is the Identity
            ref1.forEachManifoldWithOther(calcManifoldJacobian,ref2);

             return D;
        }
    }
    /**
     * @brief Calculates the transformation Jacobian that transform from reference r1 to reference r2
     * 
     * This method is used in the special case where the expected value of the Gaussian is reference r1.
     * 
     * @tparam Manifold The type of the manifold
     * @tparam DOF The degree of freedom of the manifold
     * @param ref1 The base reference
     * @param ref2 The target reference
     * @return MatrixType<ScalarOf<Manifold>,DOF,DOF> The jacobian that transforms the covariance from the base to target reference
     */
    template <typename ManifoldType, int DOF = DOFOf<ManifoldType>>
    inline AutoMatrixType<ScalarOf<ManifoldType>, DOF, DOF> transformReferenceJacobian(const ManifoldType &ref1, const ManifoldType &ref2)
    {
         if constexpr (!std::is_base_of<CompoundManifold, ManifoldType>::value)
        {
            return extractJacobi(ref1 + getDerivator<DOF>() - ref2);
        }
        else
        {
            //Definition of the Jacobian of the Boxplus Function
            AutoMatrixType<ScalarOf<ManifoldType>, DOF, DOF> D = D.Identity(DOF, DOF);
            //Counter for the current dof at iterating
            int dof = 0;
            //calculate the part of the Jacobian which belongs to the Manifold
            auto calcManifoldJacobian = [&](auto &manifold, auto & other_manifold) {
                int constexpr curDOF = DOFOf<decltype(manifold)>;
                //Calculate the Jacobian of the Boxplus Function
                D.template block<curDOF, curDOF>(dof, dof) = transformReferenceJacobian(manifold, other_manifold);
                dof += curDOF;
            };
            //apply on each manifold, for vectors the Jacobian is the Identity
            ref1.forEachManifoldWithOther(calcManifoldJacobian,ref2);

            return D;
        }
    }

}