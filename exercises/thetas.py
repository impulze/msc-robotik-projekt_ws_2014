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

def at(arg):
	return numpy.arctan(arg)

def at2(arg1, arg2):
	return numpy.arctan2(arg1, arg2)

# 25.
#alpha = 155.13
#beta = 70.18
#gamma = 72.23
#r6 = numpy.matrix([[438.79], [-235.37], [643.39]])
# 4.
alpha = 100.0
beta = 49.2
gamma = 93.0
r6 = numpy.matrix([[380.0], [100.0], [600.0]])

a1 = 100.0
d1 = 350.0
a2 = 250.0
a3 = 130.0
d4 = 250.0
d6 = 85.0
l3 = numpy.sqrt(a3 * a3 + d4 * d4)

ax = nc(dr(alpha)) * ns(dr(beta)) * nc(dr(gamma)) + ns(dr(alpha)) * ns(dr(gamma))
ay = nc(dr(alpha)) * ns(dr(beta)) * ns(dr(gamma)) - ns(dr(alpha)) * nc(dr(gamma))
az = nc(dr(alpha)) * nc(dr(beta))
a = numpy.matrix([[ax], [ay], [az]])
r4 = r6 - d6 * a
theta1 = rd(at2(r4.item(1), r4.item(0)))
print(theta1)


R2 = r4.item(0) * r4.item(0) + r4.item(1) * r4.item(1)
R = numpy.sqrt(R2)
q2 = (r4.item(2) - d1) * (r4.item(2) - d1) + (R - a1) * (R - a1)
q = numpy.sqrt(q2)
alpha_theta2 = at2(r4.item(2) - d1, R - a1)
cos_beta_theta2 = (a2 * a2 + q * q - l3 * l3) / (2 * a2 * q)
beta_theta2 = numpy.arccos(cos_beta_theta2)
theta2 = 90 - rd(alpha_theta2) - rd(beta_theta2)
print(theta2)

delta_theta3 = at2(d4, a3)
cos_gamma_theta3 = (l3 * l3 + a2 * a2 - q * q) / (2 * l3 * a2)
gamma_theta3 = numpy.arccos(cos_gamma_theta3)
theta3 = -rd(gamma_theta3) - rd(delta_theta3) + 270
print(theta3)
