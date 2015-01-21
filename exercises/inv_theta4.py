#!/usr/bin/env python

import numpy

def nc(arg):
	return numpy.cos(arg)

def ns(arg):
	return numpy.sin(arg)

def dr(arg):
	return numpy.deg2rad(arg)

def rd(arg):
	return numpy.rad2deg(arg)

def ncd(arg):
	return nc(dr(arg))

def nsd(arg):
	return ns(dr(arg))

def at(arg):
	return numpy.arctan(arg)

def at2(arg1, arg2):
	return numpy.arctan2(arg1, arg2)

def calc_all_pos_matrix(s, c, sa, ca, d, a):
	l = numpy.matrix([
		[c, -s, 0, 0],
		[s, c, 0, 0],
		[0, 0, 1, d],
		[0, 0, 0, 1]
	])

	r = numpy.matrix([
		[1, 0, 0, a],
		[0, ca, -sa, 0],
		[0, sa, ca, 0],
		[0, 0, 0, 1]
	])

	return l.dot(r)

a1 = 100.0
d1 = 350.0
a2 = 250.0
d2 = 0.0
a3 = 130.0
d3 = 0.00
a4 = 0.0
d4 = 250.0
a5 = 0.0
d5 = 0.0
a6 = 0.0
d6 = 85.0
l3 = numpy.sqrt(a3 * a3 + d4 * d4)
arm = 1 # right
elbow = 1 # above
hand = 1 # flip

def m01(theta):
	global a1
	global d1

	s = ns(theta)
	c = nc(theta)

	sa = -1
	ca = 0

	return calc_all_pos_matrix(s, c, sa, ca, d1, a1)

def m12(theta):
	global a2
	global d2

	s = -1 * nc(theta)
	c = ns(theta)
	sa = 0
	ca = 1

	return calc_all_pos_matrix(s, c, sa, ca, d2, a2)

def m23(theta):
	global a3
	global d3

	s = -1 * nc(theta)
	c = ns(theta)
	sa = -1
	ca = 0

	return calc_all_pos_matrix(s, c, sa, ca, d3, a3)

def m34(theta):
	global a4
	global d4

	s = ns(theta)
	c = nc(theta)
	sa = 1
	ca = 0

	return calc_all_pos_matrix(s, c, sa, ca, d4, a4)

def m45(theta):
	global a5
	global d5

	s = ns(theta)
	c = nc(theta)
	sa = -1
	ca = 0

	return calc_all_pos_matrix(s, c, sa, ca, d5, a5)

def m56(theta):
	global a6
	global d6

	s = -1 * ns(theta)
	c = -1 * nc(theta)
	sa = 0
	ca = 1

	return calc_all_pos_matrix(s, c, sa, ca, d6, a6)

def m6t(d, a):
	s = 0
	c = 1
	sa = 0
	ca = 1

	return calc_all_pos_matrix(s, c, sa, ca, d, a)

def m10(theta1):
	global a1
	global d1

	s1 = ns(theta1)
	c1 = nc(theta1)

	return numpy.matrix([
		[c1, s1, 0, -a1],
		[0, 0, -1, d1],
		[-s1, c1, 0, 0],
		[0, 0, 0, 1]
	])

def m21(theta2):
	global a2
	global d2

	s2 = ns(theta2)
	c2 = nc(theta2)

	return numpy.matrix([
		[s2, -c2, 0, -a2],
		[c2, s2, 0, 0],
		[0, 0, 1, 0],
		[0, 0, 0, 1]
	])

def m32(theta3):
	global a3
	global d3

	s3 = ns(theta3)
	c3 = nc(theta3)

	return numpy.matrix([
		[s3, -c3, 0, -a3],
		[0, 0, -1, 0],
		[c3, s3, 0, 0],
		[0, 0, 0, 1]
	])

def inv_m6t(dtool):
	return numpy.matrix([
		[1, 0, 0, 0],
		[0, 1, 0, 0],
		[0, 0, 1, -dtool],
		[0, 0, 0, 1]
	])

# 25.
alpha = 155.13
beta = 70.18
gamma = 72.23
r6 = numpy.matrix([[438.79], [-235.37], [643.39]])
# 4.
#alpha = 100.0
#beta = 49.2
#gamma = 93.0
#r6 = numpy.matrix([[380.0], [100.0], [600.0]])

ax = ncd(alpha) * nsd(beta) * ncd(gamma) + nsd(alpha) * nsd(gamma)
ay = ncd(alpha) * nsd(beta) * nsd(gamma) - nsd(alpha) * ncd(gamma)
az = ncd(alpha) * ncd(beta)
a = numpy.matrix([[ax], [ay], [az]])
r4 = r6 - d6 * a
theta1 = at2(arm * r4.item(1), arm * r4.item(0))
theta1_deg = rd(theta1)
print("Theta 1")
print(theta1_deg)

R2 = r4.item(0) * r4.item(0) + r4.item(1) * r4.item(1)
R = numpy.sqrt(R2)
q2 = (r4.item(2) - d1) * (r4.item(2) - d1) + (R - a1) * (R - a1)
q = numpy.sqrt(q2)
alpha_theta2 = at2(r4.item(2) - d1, R - a1)
cos_beta_theta2 = (a2 * a2 + q * q - l3 * l3) / (2 * a2 * q)
beta_theta2 = numpy.arccos(cos_beta_theta2)
theta2 = numpy.pi / 2.0 - alpha_theta2 - elbow * beta_theta2
theta2_deg = rd(theta2)
print("Theta 2")
print(theta2_deg)

delta_theta3 = at2(d4, a3)
cos_gamma_theta3 = (l3 * l3 + a2 * a2 - q * q) / (2 * l3 * a2)
gamma_theta3 = numpy.arccos(cos_gamma_theta3)
theta3 = -elbow * gamma_theta3 - delta_theta3 + 3 * numpy.pi / 2.0
theta3_deg = rd(theta3)
print("Theta 3")
print(theta3_deg)

s1 = ns(theta1)
c1 = nc(theta1)
s2 = ns(theta2)
c2 = nc(theta2)
s3 = ns(theta3)
c3 = nc(theta3)

s23 = s2 * c3 + c2 * s3
c23 = c2 * c3 - s2 * s3

theta4 = at2(hand * (-ax * s1 + ay * c1), hand * (ax * c1 * c23 + ay * s1 * c23 - az * s23))
theta4_deg = rd(theta4)
print("Theta 4")
print(theta4_deg)

c4 = nc(theta4)
s4 = ns(theta4)

theta5 = at2(ax * (c1 * c23 * c4 - s1 * s4) + ay * (s1 * c23 * c4 + c1 * s4) - az * s23 * c4,
             ax * c1 * s23 + ay * s1 * s23 + az * c23)
theta5_deg = rd(theta5)
print("Theta 5")
print(theta5_deg)

nx = ncd(beta) * ncd(gamma)
ny = ncd(beta) * nsd(gamma)
nz = -nsd(beta)
sx = nsd(alpha) * nsd(beta) * ncd(gamma) - ncd(alpha) * nsd(gamma)
sy = nsd(alpha) * nsd(beta) * nsd(gamma) + ncd(alpha) * ncd(gamma)
sz = nsd(alpha) * ncd(beta)

theta6 = at2(nx * (-c1 * c23 * s4 - s1 * c4) + ny * (-s1 * c23 * s4 + c1 * c4) + nz * s23 * s4,
             sx * (-c1 * c23 * s4 - s1 * c4) + sy * (-s1 * c23 * s4 + c1 * c4) + sz * s23 * s4)
theta6_deg = rd(theta6)
print("Theta 6")
print(theta6_deg)

#theta6_deg = 100.0
#theta6 = dr(theta6_deg)

new_m01 = m01(theta1)
new_m12 = m12(theta2)
new_m23 = m23(theta3)
new_m34 = m34(theta4)
new_m45 = m45(theta5)
new_m56 = m56(theta6)
new_m6t = m6t(0, 0)
new_m0t = new_m01.dot(new_m12).dot(new_m23).dot(new_m34).dot(new_m45).dot(new_m56).dot(new_m6t)

new_nx = new_m0t.item(0, 0)
new_ny = new_m0t.item(1, 0)
new_nz = new_m0t.item(2, 0)
new_sz = new_m0t.item(2, 1)
new_az = new_m0t.item(2, 2)
new_alpha = at2(new_sz, new_az)
new_beta = at2(-new_nz, new_sz / ns(new_alpha))
new_gamma = at2(new_ny, new_nx)

new_point = numpy.matrix([[0], [0], [0], [1]])

print "Original position:"
print r6

print "New position:"
print new_m0t.dot(new_point)

print "Original alpha, beta, gamma:"
print alpha
print beta
print gamma

print "New alpha, beta, gamma:"
print rd(new_alpha)
print rd(new_beta)
print rd(new_gamma)
