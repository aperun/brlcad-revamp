#ifndef IDLINEARMATHINTERFACE_HPP_
#define IDLINEARMATHINTERFACE_HPP_

#include <cstdlib>

#include "../IDConfig.hpp"

#include "../../LinearMath/btMatrix3x3.h"
#include "../../LinearMath/btVector3.h"
#include "../../LinearMath/btMatrixX.h"

namespace btInverseDynamics {
class vec3;
class vecx;
class mat33;
typedef btMatrixX<idScalar> matxx;

class vec3 : public btVector3 {
public:
	vec3() : btVector3() {}
	vec3(const btVector3& btv) { *this = btv; }
	idScalar& operator()(int i) { return (*this)[i]; }
	const idScalar& operator()(int i) const { return (*this)[i]; }
	const int size() const { return 3; }
	const vec3& operator=(const btVector3& rhs) {
		*static_cast<btVector3*>(this) = rhs;
		return *this;
	}
};

class mat33 : public btMatrix3x3 {
public:
	mat33() : btMatrix3x3() {}
	mat33(const btMatrix3x3& btm) { *this = btm; }
	idScalar& operator()(int i, int j) { return (*this)[i][j]; }
	const idScalar& operator()(int i, int j) const { return (*this)[i][j]; }
	const mat33& operator=(const btMatrix3x3& rhs) {
		*static_cast<btMatrix3x3*>(this) = rhs;
		return *this;
	}
	friend mat33 operator*(const idScalar& s, const mat33& a);
	friend mat33 operator/(const mat33& a, const idScalar& s);
};

inline mat33 operator/(const mat33& a, const idScalar& s) { return a * (1.0 / s); }

inline mat33 operator*(const idScalar& s, const mat33& a) { return a * s; }

class vecx : public btVectorX<idScalar> {
public:
	vecx(int size) : btVectorX(size) {}
	const vecx& operator=(const btVectorX<idScalar>& rhs) {
		*static_cast<btVectorX*>(this) = rhs;
		return *this;
	}

	idScalar& operator()(int i) { return (*this)[i]; }
	const idScalar& operator()(int i) const { return (*this)[i]; }

	friend vecx operator*(const vecx& a, const idScalar& s);
	friend vecx operator*(const idScalar& s, const vecx& a);

	friend vecx operator+(const vecx& a, const vecx& b);
	friend vecx operator-(const vecx& a, const vecx& b);
	friend vecx operator/(const vecx& a, const idScalar& s);
};

inline vecx operator*(const vecx& a, const idScalar& s) {
	vecx result(a.size());
	for (int i = 0; i < result.size(); i++) {
		result(i) = a(i) * s;
	}
	return result;
}
inline vecx operator*(const idScalar& s, const vecx& a) { return a * s; }
inline vecx operator+(const vecx& a, const vecx& b) {
	vecx result(a.size());
	// TODO: error handling for a.size() != b.size()??
	if (a.size() != b.size()) {
		error_message("size missmatch. a.size()= %d, b.size()= %d\n", a.size(), b.size());
		abort();
	}
	for (int i = 0; i < a.size(); i++) {
		result(i) = a(i) + b(i);
	}

	return result;
}

inline vecx operator-(const vecx& a, const vecx& b) {
	vecx result(a.size());
	// TODO: error handling for a.size() != b.size()??
	if (a.size() != b.size()) {
		error_message("size missmatch. a.size()= %d, b.size()= %d\n", a.size(), b.size());
		abort();
	}
	for (int i = 0; i < a.size(); i++) {
		result(i) = a(i) - b(i);
	}
	return result;
}
inline vecx operator/(const vecx& a, const idScalar& s) {
	vecx result(a.size());
	for (int i = 0; i < result.size(); i++) {
		result(i) = a(i) / s;
	}

	return result;
}
}

#endif  // IDLINEARMATHINTERFACE_HPP_
