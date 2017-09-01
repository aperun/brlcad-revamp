#include "IDMath.hpp"

#include <cmath>
#include <limits>

namespace btInverseDynamics {
static const idScalar kIsZero = 5 * std::numeric_limits<idScalar>::epsilon();
// requirements for axis length deviation from 1.0
// experimentally set from random euler angle rotation matrices
static const idScalar kAxisLengthEpsilon = 10 * kIsZero;

void setZero(vec3 &v) {
	v(0) = 0;
	v(1) = 0;
	v(2) = 0;
}

void setZero(vecx &v) {
	for (int i = 0; i < v.size(); i++) {
		v(i) = 0;
	}
}

void setZero(mat33 &m) {
	m(0, 0) = 0;
	m(0, 1) = 0;
	m(0, 2) = 0;
	m(1, 0) = 0;
	m(1, 1) = 0;
	m(1, 2) = 0;
	m(2, 0) = 0;
	m(2, 1) = 0;
	m(2, 2) = 0;
}

idScalar maxAbs(const vecx &v) {
	idScalar result = 0.0;
	for (int i = 0; i < v.size(); i++) {
		const idScalar tmp = std::fabs(v(i));
		if (tmp > result) {
			result = tmp;
		}
	}
	return result;
}

idScalar maxAbs(const vec3 &v) {
	idScalar result = 0.0;
	for (int i = 0; i < 3; i++) {
		const idScalar tmp = std::fabs(v(i));
		if (tmp > result) {
			result = tmp;
		}
	}
	return result;
}

mat33 transformX(const idScalar &alpha) {
	mat33 T;
	const idScalar cos_alpha = std::cos(alpha);
	const idScalar sin_alpha = std::sin(alpha);
	// [1  0 0]
	// [0  c s]
	// [0 -s c]
	T(0, 0) = 1.0;
	T(0, 1) = 0.0;
	T(0, 2) = 0.0;

	T(1, 0) = 0.0;
	T(1, 1) = cos_alpha;
	T(1, 2) = sin_alpha;

	T(2, 0) = 0.0;
	T(2, 1) = -sin_alpha;
	T(2, 2) = cos_alpha;

	return T;
}

mat33 transformY(const idScalar &beta) {
	mat33 T;
	const idScalar cos_beta = std::cos(beta);
	const idScalar sin_beta = std::sin(beta);
	// [c 0 -s]
	// [0 1  0]
	// [s 0  c]
	T(0, 0) = cos_beta;
	T(0, 1) = 0.0;
	T(0, 2) = -sin_beta;

	T(1, 0) = 0.0;
	T(1, 1) = 1.0;
	T(1, 2) = 0.0;

	T(2, 0) = sin_beta;
	T(2, 1) = 0.0;
	T(2, 2) = cos_beta;

	return T;
}

mat33 transformZ(const idScalar &gamma) {
	mat33 T;
	const idScalar cos_gamma = std::cos(gamma);
	const idScalar sin_gamma = std::sin(gamma);
	// [ c s 0]
	// [-s c 0]
	// [ 0 0 1]
	T(0, 0) = cos_gamma;
	T(0, 1) = sin_gamma;
	T(0, 2) = 0.0;

	T(1, 0) = -sin_gamma;
	T(1, 1) = cos_gamma;
	T(1, 2) = 0.0;

	T(2, 0) = 0.0;
	T(2, 1) = 0.0;
	T(2, 2) = 1.0;

	return T;
}

mat33 tildeOperator(const vec3 &v) {
	mat33 m;
	m(0, 0) = 0.0;
	m(0, 1) = -v(2);
	m(0, 2) = v(1);
	m(1, 0) = v(2);
	m(1, 1) = 0.0;
	m(1, 2) = -v(0);
	m(2, 0) = -v(1);
	m(2, 1) = v(0);
	m(2, 2) = 0.0;
	return m;
}

void getVecMatFromDH(idScalar theta, idScalar d, idScalar a, idScalar alpha, vec3 *r, mat33 *T) {
	const idScalar sa = std::sin(alpha);
	const idScalar ca = std::cos(alpha);
	const idScalar st = std::sin(theta);
	const idScalar ct = std::cos(theta);

	(*r)(0) = a;
	(*r)(1) = -sa * d;
	(*r)(2) = ca * d;

	(*T)(0, 0) = ct;
	(*T)(0, 1) = -st;
	(*T)(0, 2) = 0.0;

	(*T)(1, 0) = st * ca;
	(*T)(1, 1) = ct * ca;
	(*T)(1, 2) = -sa;

	(*T)(2, 0) = st * sa;
	(*T)(2, 1) = ct * sa;
	(*T)(2, 2) = ca;
}

void bodyTParentFromAxisAngle(const vec3 &axis, const idScalar &angle, mat33 *T) {
	const idScalar c = cos(angle);
	const idScalar s = -sin(angle);
	const idScalar one_m_c = 1.0 - c;

	const idScalar &x = axis(0);
	const idScalar &y = axis(1);
	const idScalar &z = axis(2);

	(*T)(0, 0) = x * x * one_m_c + c;
	(*T)(0, 1) = x * y * one_m_c - z * s;
	(*T)(0, 2) = x * z * one_m_c + y * s;

	(*T)(1, 0) = x * y * one_m_c + z * s;
	(*T)(1, 1) = y * y * one_m_c + c;
	(*T)(1, 2) = y * z * one_m_c - x * s;

	(*T)(2, 0) = x * z * one_m_c - y * s;
	(*T)(2, 1) = y * z * one_m_c + x * s;
	(*T)(2, 2) = z * z * one_m_c + c;
}

bool isPositiveDefinite(const mat33 &m) {
	// test if all upper left determinants are positive
	if (m(0, 0) <= 0) {  // upper 1x1
		return false;
	}
	if (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0) <= 0) {  // upper 2x2
		return false;
	}
	if ((m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) -
		 m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
		 m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0))) < 0) {
		return false;
	}
	return true;
}

bool isPositiveSemiDefinite(const mat33 &m) {
	// test if all upper left determinants are positive
	if (m(0, 0) < 0) {  // upper 1x1
		return false;
	}
	if (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0) < 0) {  // upper 2x2
		return false;
	}
	if ((m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) -
		 m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
		 m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0))) < 0) {
		return false;
	}
	return true;
}

bool isPositiveSemiDefiniteFuzzy(const mat33 &m) {
	// test if all upper left determinants are positive
	if (m(0, 0) < -kIsZero) {  // upper 1x1
		return false;
	}
	if (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0) < -kIsZero) {  // upper 2x2
		return false;
	}
	if ((m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) -
		 m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
		 m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0))) < -kIsZero) {
		return false;
	}
	return true;
}

idScalar determinant(const mat33 &m) {
	return m(0, 0) * m(1, 1) * m(2, 2) + m(0, 1) * m(1, 2) * m(2, 0) + m(0, 2) * m(1, 0) * m(2, 1) -
		   m(0, 2) * m(1, 1) * m(2, 0) - m(0, 0) * m(1, 2) * m(2, 1) - m(0, 1) * m(1, 0) * m(2, 2);
}

bool isValidInertiaMatrix(const mat33 &I, const int index, bool has_fixed_joint) {
	// TODO(Thomas) do we really want this?
	//			  in cases where the inertia tensor about the center of mass is zero,
	//			  the determinant of the inertia tensor about the joint axis is almost
	//			  zero and can have a very small negative value.
	if (!isPositiveSemiDefiniteFuzzy(I)) {
		error_message("invalid inertia matrix for body %d, not positive definite "
					  "(fixed joint)\n",
					  index);
		error_message("matrix is:\n"
					  "[%.20e %.20e %.20e;\n"
					  "%.20e %.20e %.20e;\n"
					  "%.20e %.20e %.20e]\n",
					  I(0, 0), I(0, 1), I(0, 2), I(1, 0), I(1, 1), I(1, 2), I(2, 0), I(2, 1),
					  I(2, 2));

		return false;
	}

	// check triangle inequality, must have I(i,i)+I(j,j)>=I(k,k)
	if (!has_fixed_joint) {
		if (I(0, 0) + I(1, 1) < I(2, 2)) {
			error_message("invalid inertia tensor for body %d, I(0,0) + I(1,1) < I(2,2)\n", index);
			error_message("matrix is:\n"
						  "[%.20e %.20e %.20e;\n"
						  "%.20e %.20e %.20e;\n"
						  "%.20e %.20e %.20e]\n",
						  I(0, 0), I(0, 1), I(0, 2), I(1, 0), I(1, 1), I(1, 2), I(2, 0), I(2, 1),
						  I(2, 2));
			return false;
		}
		if (I(0, 0) + I(1, 1) < I(2, 2)) {
			error_message("invalid inertia tensor for body %d, I(0,0) + I(1,1) < I(2,2)\n", index);
			error_message("matrix is:\n"
						  "[%.20e %.20e %.20e;\n"
						  "%.20e %.20e %.20e;\n"
						  "%.20e %.20e %.20e]\n",
						  I(0, 0), I(0, 1), I(0, 2), I(1, 0), I(1, 1), I(1, 2), I(2, 0), I(2, 1),
						  I(2, 2));
			return false;
		}
		if (I(1, 1) + I(2, 2) < I(0, 0)) {
			error_message("invalid inertia tensor for body %d, I(1,1) + I(2,2) < I(0,0)\n", index);
			error_message("matrix is:\n"
						  "[%.20e %.20e %.20e;\n"
						  "%.20e %.20e %.20e;\n"
						  "%.20e %.20e %.20e]\n",
						  I(0, 0), I(0, 1), I(0, 2), I(1, 0), I(1, 1), I(1, 2), I(2, 0), I(2, 1),
						  I(2, 2));
			return false;
		}
	}
	// check positive/zero diagonal elements
	for (int i = 0; i < 3; i++) {
		if (I(i, i) < 0) {  // accept zero
			error_message("invalid inertia tensor, I(%d,%d)= %e <0\n", i, i, I(i, i));
			return false;
		}
	}
	// check symmetry
	if (std::fabs(I(1, 0) - I(0, 1)) > kIsZero) {
		error_message("invalid inertia tensor for body %d I(1,0)!=I(0,1). I(1,0)-I(0,1)= "
					  "%e\n",
					  index, I(1, 0) - I(0, 1));
		return false;
	}
	if (std::fabs(I(2, 0) - I(0, 2)) > kIsZero) {
		error_message("invalid inertia tensor for body %d I(2,0)!=I(0,2). I(2,0)-I(0,2)= "
					  "%e\n",
					  index, I(2, 0) - I(0, 2));
		return false;
	}
	if (std::fabs(I(1, 2) - I(2, 1)) > kIsZero) {
		error_message("invalid inertia tensor body %d I(1,2)!=I(2,1). I(1,2)-I(2,1)= %e\n", index,
					  I(1, 2) - I(2, 1));
		return false;
	}
	return true;
}

bool isValidTransformMatrix(const mat33 &m) {
#define print_mat(x)																			   \
	error_message("matrix is [%e, %e, %e; %e, %e, %e; %e, %e, %e]\n", x(0, 0), x(0, 1), x(0, 2),   \
				  x(1, 0), x(1, 1), x(1, 2), x(2, 0), x(2, 1), x(2, 2))

	// check for unit length column vectors
	for (int i = 0; i < 3; i++) {
		const idScalar length_minus_1 =
			std::fabs(m(0, i) * m(0, i) + m(1, i) * m(1, i) + m(2, i) * m(2, i) - 1.0);
		if (length_minus_1 > kAxisLengthEpsilon) {
			error_message("Not a valid rotation matrix (column %d not unit length)\n"
						  "column = [%.18e %.18e %.18e]\n"
						  "length-1.0= %.18e\n",
						  i, m(0, i), m(1, i), m(2, i), length_minus_1);
			print_mat(m);
			return false;
		}
	}
	// check for orthogonal column vectors
	if (std::fabs(m(0, 0) * m(0, 1) + m(1, 0) * m(1, 1) + m(2, 0) * m(2, 1)) > kAxisLengthEpsilon) {
		error_message("Not a valid rotation matrix (columns 0 and 1 not orthogonal)\n");
		print_mat(m);
		return false;
	}
	if (std::fabs(m(0, 0) * m(0, 2) + m(1, 0) * m(1, 2) + m(2, 0) * m(2, 2)) > kAxisLengthEpsilon) {
		error_message("Not a valid rotation matrix (columns 0 and 2 not orthogonal)\n");
		print_mat(m);
		return false;
	}
	if (std::fabs(m(0, 1) * m(0, 2) + m(1, 1) * m(1, 2) + m(2, 1) * m(2, 2)) > kAxisLengthEpsilon) {
		error_message("Not a valid rotation matrix (columns 0 and 2 not orthogonal)\n");
		print_mat(m);
		return false;
	}
	// check determinant (rotation not reflection)
	if (determinant(m) <= 0) {
		error_message("Not a valid rotation matrix (determinant <=0)\n");
		print_mat(m);
		return false;
	}
	return true;
}

bool isUnitVector(const vec3 &vector) {
	return std::fabs(vector(0) * vector(0) + vector(1) * vector(1) + vector(2) * vector(2) - 1.0) <
		   kIsZero;
}

vec3 rpyFromMatrix(const mat33 &rot) {
	vec3 rpy;
	rpy(2) = std::atan2(-rot(1, 0), rot(0, 0));
	rpy(1) = std::atan2(rot(2, 0), std::cos(rpy(2)) * rot(0, 0) - std::sin(rpy(0)) * rot(1, 0));
	rpy(0) = std::atan2(-rot(2, 0), rot(2, 2));
	return rpy;
}
}
