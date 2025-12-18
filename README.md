# box-fitting
Genetic Algorithm for finding the optimal packing of n squares in a box.

#Prerequesites
To build the project, you'll need:
- The g++ compiler, supporting C++20 or ahead (you may install it in linux through `sudo apt install g++`)
- The gnuplot library (obtainable through `sudo apt install gnuplot`)


# Installation and building
This project supports UNIX systems and Windows. To install, first clone the repository through
```
git clone https://github.com/Sekqies/box-fitting
```
It is recommended to use VScode for building. If you are using the Visual Studio Code editor, you may build the project through the shortcut `CTRL+B`. Otherwise, the building commands are specified in the `.json` configuration files.

# Running
This project comes with a pre-compiled, `/build/main`. If at windows, merely run
```
./build/main.exe
```
If in linux, build the project and run
```
./build/main
```

# Usage
You may stop the rendering of the boxes by pressing `R` on your keyboard. The algorithm will run until the program is terminated, at which point it will generate you a plot of the best and average fitness for every generation. It will also download an image and pdf of the plot.

# Video
For more information on the project, you may watch the following https://youtu.be/pTiY2hepH9e

 