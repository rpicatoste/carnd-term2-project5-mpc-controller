# Self-Driving Car Nanodegree Program - Term 2
## Project 5 - MPC Control
**Ricardo Picatoste**

## Notes
The project has been compiled in Windows 10 using the "Bash on Ubuntu on Windows", generating the make file with "cmake CMakeLists.txt" and then running make. 

To set up all the libraries, I followed this forum post, with some additional steps since I was not working in Linux:

[Installing dependencies](https://discussions.udacity.com/t/how-to-install-project-dependencies-and-quizzes-for-ubuntu/304975)

I have used a tab width of 4 spaces, taking care of the matrices alignment. I hope it's ok, I prefer it like that for programming.

## Results

The car does the whole track without leaving the road. The speed is set to 40, and the controller does an excellent job keeping the car between the lanes.

To chose N and dt I just followed a thumb rule for discrete controllers applied to autonomous cars, where 10 Hz is a proper rate for the dynamics of the car. N I kept it to 10, so a second of trajectory is used for the calculations.

The hardest parts were 2: 

- To solve the problem of the latency. I had to look through the forums, and after trying the prediction of the state in different places of the code without success it finally worked. The initial problem is that I was doing the trayectory transformation of coordinates with the predicted state, leading to error.

- The next hard point was to select the order of the polynomial to fit to the desired trajectory. I tried 5 and the result was good except for after more that half circuit, the controller got apparently crazy. With an order of 3 this problem is avoided. Probably with a more careful fit of the polynomial the higher order could be used.

The last point was the tuning. With 100 as coefficient for both control actions the result is much better, avoiding oscillations in both.

