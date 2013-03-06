#!/usr/bin/python

import scipy.linalg
import numpy
import sys

def calc_coefficients(adc1, grav1, adc2, grav2, adc3, grav3):

	adcmatrix = numpy.matrix([[adc1**3, adc1**2, adc1, 1],
	    [adc2**3, adc2**2, adc2, 1],
	    [adc3**3, adc3**2, adc3, 1],
	    [6*adc2, 2, 0, 0]])

	gravmatrix = numpy.matrix([[grav1],[grav2],[grav3], [0]])

	result = scipy.linalg.solve(adcmatrix, gravmatrix);

	print("%e * x**3 + %e * x**2 + %e * x + %e\n" % (
		result[0,0], result[1,0], result[2,0], result[3,0]))

	print(gravmatrix);
	print(adcmatrix);
	print(result);



def main():
	adc1 = float(sys.argv[1])
	adc2 = float(sys.argv[2])
	adc3 = float(sys.argv[3])
	grav1 = float(sys.argv[4])
	grav2 = float(sys.argv[5])
	grav3 = float(sys.argv[6])

	calc_coefficients(adc1, grav1, adc2, grav2, adc3, grav3)



main()







