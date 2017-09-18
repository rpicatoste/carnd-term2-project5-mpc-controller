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

