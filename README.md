# Pd_Externals
A repository for my own Pd external source codes.

# author & contacts 
Cheng-i Wang, wangsix@gmail.com

# coin
coin is a flip coin object, which you can set the bernoulli experiment probability, the value for heads and the value for tails.

# scanslide & scanslide~
scanslide & scanslide~ are the same object but one in Pd rate the other in audio rate. Scanslide is a Pd version of the Max/MSP object scanslide. It basically is an IIR filter with the formula y[n] = y[n-1] + (x[n] - y[n-1])/slide. Both uphill and downramp slide value could be specified

# binaural~
binaural~ is a 3D binaural simulation object which takes a mono signal in then projects the sound onto the 3D sphere around your head based on the arguments(best with headphones). The implementation is referenced from Miller Puckette`s paper and utilized the MIT hrtf C library.

