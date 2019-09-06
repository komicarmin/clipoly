# Clipoly
Polygon clipping algorithm implementation in C. Program takes input file containing polygon data and performs Boolean operation on them.

## Usage
Run program with following CLI command:
```bash
gcc main.c -o clipoly
./clipoly [file-name].txt [operationNum]
```

## Operations
| Operation          | operationNum |
|--------------------|--------------|
| Union              | 1            |
| Intersection       | 2            |
| Difference         | 3            |
| Reverse Difference | 4            |
| XOR                | 5            |

## Input file
Input file must be encoded in UTF-8 (without BOM) encoding and polygon data must be in following format:
```
x,y,z
x,y,z
x,y,z

x,y,z
x,y,z
x,y,z

```
One empty line represents end of current polygon and start of new one.

Example:
```
643508.544956253608689,1831463.091295037884265,13.608339231677062
643507.583161425078288,1831465.501027235761285,13.569608284184639
643506.773306413320825,1831464.166073419386521,13.925623290249504

643506.773306413320825,1831464.166073419386521,13.925623290249504
643507.583161425078288,1831465.501027235761285,13.569608284184639
643506.738572752568871,1831468.103960443288088,13.479353113328044

```
