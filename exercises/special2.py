#!/usr/bin/env python

import numpy

def cos_deg(x):
	return numpy.cos(numpy.deg2rad(x))

def sin_deg(x):
	return numpy.sin(numpy.deg2rad(x))

def calc_alpha(matrix, beta):
	r21 = matrix.item((2, 1))
	r22 = matrix.item((2, 2))
	cb = cos_deg(beta)

	return numpy.rad2deg(numpy.arctan2(r21 / cb, r22 / cb))

def calc_beta(matrix):
	r20 = matrix.item((2, 0))
	r21 = matrix.item((2, 1))
	r22 = matrix.item((2, 2))

	return numpy.rad2deg(numpy.arctan2(-r20, numpy.sqrt(r21 * r21 + r22 * r22)))

def calc_gamma(matrix, beta):
	r10 = matrix.item((1, 0))
	r00 = matrix.item((0, 0))
	cb = cos_deg(beta)

	return numpy.rad2deg(numpy.arctan2(r10 / cb, r00 / cb))

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

def calc_all_rot_matrix(alpha, beta, gamma):
	sa = sin_deg(alpha)
	ca = cos_deg(alpha)
	sb = sin_deg(beta)
	cb = cos_deg(beta)
	sg = sin_deg(gamma)
	cg = cos_deg(gamma)

	rx = numpy.matrix([
		[1, 0, 0],
		[0, ca, -sa],
		[0, sa, ca]
	])

	ry = numpy.matrix([
		[cb, 0, sb],
		[0, 1, 0],
		[-sb, 0, cb]
	])

	rz = numpy.matrix([
		[cg, -sg, 0],
		[sg, cg, 0],
	])

	return rz.dot(ry).dot(rx)

def m1(theta, d, a):
	s = sin_deg(theta)
	c = cos_deg(theta)
	sa = 1
	ca = 0

	return calc_all_pos_matrix(s, c, sa, ca, d, a)

def m2(theta, d, a):
	s = -cos_deg(theta)
	c = -sin_deg(theta)
	sa = 0
	ca = 1

	return calc_all_pos_matrix(s, c, sa, ca, d, a)

def m3(theta, d, a):
	s = sin_deg(theta)
	c = cos_deg(theta)
	sa = 1
	ca = 0

	return calc_all_pos_matrix(s, c, sa, ca, d, a)

def m4(theta, d, a):
	s = sin_deg(theta)
	c = cos_deg(theta)
	sa = 0
	ca = 1

	return calc_all_pos_matrix(s, c, sa, ca, d, a)

def mt(d, a):
	s = 0
	c = 1
	sa = 0
	ca = 1

	return calc_all_pos_matrix(s, c, sa, ca, d, a)

def max_theta(current):
	return {
		1: (-150, 150),
		2: (-150, 150),
		3: (-150, 150),
		4: (-0, 0)
	}[current]

dh_parameters = {
	'd1':0, 'a1':0,
	'd2':0, 'a2':0,
	'd3':0, 'a3':0,
	'd4':0, 'a4':0
}

def set_dh_parameter(parameter, value):
	if parameter[:-1] == 'theta':
		_max_theta = max_theta(int(parameter[-1]))
		if value < _max_theta[0] or value > _max_theta[1]:
			print "theta '%s' not within range (%s but it's %s)" % (parameter, value, _max_theta)
			return

	dh_parameters[parameter] = value

def main():
	import sys

	numpy.set_printoptions(suppress = True)

	point_used = None

	print "Enter 'quit' to quit the application"
	print "Enter 'show' to show the current result"
	print "You can enter up to 5 Denavit-Hartenberg parameter sets (d4=, a1=, theta6=)"
	print "And you can enter the point to move (px=, py=, pz=)"

	while True:
		try:
			line = sys.stdin.readline()
		except KeyboardInterrupt:
			break

		if not line:
			break

		if line[:-1] == 'quit':
			sys.exit(0)
		elif line[:-1] == 'show':
			for a in [1,2,3,4]:
				if ('theta' + str(a)) not in dh_parameters:
					set_dh_parameter('theta' + str(a), 0)

			_m1 = m1(dh_parameters['theta1'], dh_parameters['d1'], dh_parameters['a1'])
			_m2 = m2(dh_parameters['theta2'], dh_parameters['d2'], dh_parameters['a2'])
			_m3 = m3(dh_parameters['theta3'], dh_parameters['d3'], dh_parameters['a3'])
			_m4 = m4(dh_parameters['theta4'], dh_parameters['d4'], dh_parameters['a4'])
			_mt = mt(0, 0)
			point = numpy.matrix([[dh_parameters['px']], [dh_parameters['py']], [dh_parameters['pz']], [1]])
			print _m1.dot(point)
			print _m1.dot(_m2).dot(point)
			print _m1.dot(_m2).dot(_m3).dot(point)
			print _m1.dot(_m2).dot(_m3).dot(_m4).dot(point)
			"""
			beta = calc_beta(m02)
			alpha = calc_alpha(m02, beta)
			gamma = calc_gamma(m02, beta)
			print 'Alpha: %f' % alpha	
			print 'Beta: %f' % beta
			print 'Gamma: %f' % gamma
			m0t = m02.dot(_mt)
			result = m0t.dot(point)
			print 'Position: (x: %f, y: %f, z: %f)' % (result.item(0), result.item(1), result.item(2))
			"""
			continue

		parameters = line[:-1]

		if line[0] == 'd' or line[0] == 'a' or line[0] == 'p':
			index_index = 1

			if line[0] == 'p' and (line[1] != 'x' and line[1] != 'y' and line[1] != 'z'):
				print "I don't understand the input '%s'" % line[:-1]
				continue

		elif line[0:5] == 'theta':
			index_index = 5
		else:
			print "I don't understand the input '%s'" % line[:-1]
			continue

		if line[index_index + 1] == '=' and (
			line[index_index + 2:-1].isdigit() or
			line[index_index + 2] == '-' and line[index_index + 3:-1].isdigit()):
			print "'%s' is '%s'" % (line[:index_index + 1], line[index_index + 2:-1])
			new_value = int(line[index_index + 2:-1])

			if line[0:5] == 'theta':
				which_theta = int(line[index_index])

				if new_value < max_theta(which_theta)[0]:
					print "Theta lower than allowed value (%d)" % (max_theta(which_theta)[0])
				elif new_value > max_theta(which_theta)[1]:
					print "Theta higher than allowed value (%d)" % (max_theta(which_theta)[1])
				else:
					set_dh_parameter(line[0:index_index + 1], new_value)
			else:
				set_dh_parameter(line[0:index_index + 1], new_value)

if __name__ == '__main__':
	main()
