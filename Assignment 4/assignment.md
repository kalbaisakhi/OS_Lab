# CS39002 Operating Systems Laboratory

Spring 2025

## Lab Assignment: 4

Date of submission: 29–Jan –2025

## Inter-process communication using pipes

In this assignment, you design a multi-process application that lets a user play the Foodoku puzzle in an interactive manner. If you do not know what the rules for a standard9×9Foodoku are, here are a sample Foodoku puzzle and its solution.


![](https://web-api.textin.com/ocr_image/external/df9cde6a2a7ded04.jpg)


![](https://web-api.textin.com/ocr_image/external/29fabfb427813380.jpg)

Call each3×3region (enclosed by bold lines in the above figure) a block. The objective of the game is to fill the empty cells by the digits 1 through 9 such that no block or no row or no column contains repetitions.Number the nine blocks and the nine cells in each block as 0–8 in the row-major fashion.

In our application program, the user interface is launched by running a coordinator program from a shell. The coordinator forks nine child processes. Each child process handles one block, and launches an xterm for the display of that block and of all error messages associated with that block. The codes for the coordinator and for each block are to be written in two files coordinator.c and block.c. The coordinator (parent) creates nine pipes for communicating with nine child processes. The blocks need to communicate with one another. The same pipes are used for that purpose too. During the game, each block B communicates with four other blocks: two in the same row as B, and two other in the same column as B. Let us call these four blocks the (row and column) neighbors of B. During the forking of a block B, the parent passes, as command-line arguments, the block number for B (an integer in the range 0–8), the two pipe descriptors for communicating with B, and the write ends of the pipes of the four neighbors of B.

All communications through the pipes must happen using the high-level printf and scanf functions (or by cin and cout). A block does not directly communicate with the user. It only receives messages from the coordinator and from its four neighbors. So each block process can permanently close its stdin, and dup() the read end of its pipe to its stdin. The descriptor for stdout of a block cannot be permanently closed. Each block process needs to draw and redraw its block and to print error messages, in that block’s xterm. For this purpose, the original stdout is to be used. For communicating with a neighbor, printf (or cout) should be used, but this time, the printed stuff will travel through the pipe of the neighbor. So the stdout of each block process will switch between screen printing and pipe printing. For the coordinator, all inputs are from the user, and the original stdin will work throughout. But its stdout will switch between the original version (for printing to the coordinator window) and the write ends of the pipes of the block processes (for passing commands). Use dup() appropriately to ensure this.

The commands used in the game are summarized below. Each command is a letter followed by the requisite number of int arguments.


| h  | Print a help message (including the numbering scheme of the blocks and the cells in each block).  |
| -- | -- |
| n  | Launch a new puzzle. A program boardgen.c is supplied to you. #include this file in the code for the coordinator. A function newboard(A, S) is implemented in boardgen.c. Here, A and S are 9×9int arrays for storing the puzzle and its solution, respectively. Empty cells in A[9][9] are written as 0’s. The solution does not contain any empty cell. The call newboard() populates these two arrays by a random-looking Foodoku puzzle. After the function returns, the coordinator sends the blocks of A[9][9] to the child processes via their respective pipes. Each child knows only its own block, and nothing about the other blocks.  |
| p b c d  | Request to put (place/replace) digit d at the c-th cell of the b-th block. The cell may be empty (place) or already filled by the user (replace). The coordinator checks whether b, c, and d are in valid ranges. If not, it sends an immediate error message to the user, and proceeds no further. If b, c, d are all valid, the coordinator sends a request p c d to block b. The coordinator does not check whether placing or replacing the digit in the indicated cell results in any conflict.  |
| s  | Show the solution of the current puzzle. This is the same as sending the blocks of the solution S[9][9] (instead of A[9][9] as for the command n) to the block processes.  |
| q  | Send a quit command to the child processes, wait for them to exit, and finally exit itself.  |


## Commands to each block


| n  | This command originates from the coordinator, and is followed by nine integers standing for the entries of the corresponding 3×3 block. An empty cell is denoted by 0. Upon receiving these nine digits, the block populates two private  arrays with these entries: A[3][3] for storing the block of the original puzzle, and B[3][3] for storing the current state of the block.  |
| -- | -- |
| p c d  | The cell number c and the digit d follow this command. This command again comes from the coordinator to the block b as a response to the user command p b c d. The block first verifies from its own copy of A[3][3] whether the user requests to replace a digit in the original puzzle. If the user does that, this causes the error Read-only cell. If not, the block checks whether d is already present in some cell of its own copy of B[3][3]. If so, it causes a Block conflict error. If not, the block sends a row-check request (see r below) to its two row neighbors. Each neighbor responds by sending 0 (no error) or a suitable non-zero integer (Row conflict error). If there is no error reported by the row neighbors, the block finally sends column-check requests (see c below) to its column neighbors. Each neighbor responds by sending 0 (no error) or a suitable non-zero integer (Column conflict error). Notice that the above checks are with respect to consistency in the current board. No efforts should be made to verify correctness of user inputs with respect to the solution. Indeed, the coordinator does not send the blocks of S[9][9] to the block processes (unless the user gives the command s, in which case the complete solution is received from the coordinator, all cells become read-only, and the user cannot proceed with the current game). If there is no error so far, the digit d is placed in the requested cell c, the local array B[3][3] is updated and redrawn in the xterm. If there is an error, an appropriate message is printed below the current block. In order that the user can read that message, the block process waits for some time (like two seconds), and then redraws the earlier block (now B[3][3] undergoes no change).  |
| r i d  | This request comes from a row neighbor, and is followed by a row number i and a digit d. In order to serve this command, the block process looks at the i-th row of its B[3][3], and responds by sending 0 or a Row conflict notification. The block process does not know from which row neighbor the request has originated. So the request must also be accompanied by an identification (write pipe-descriptor) of the requesting block.  |
| c j d  | This is similar to r with the exception that now the requested digit d is checked for conflict in the j-th column. The response is 0 or a Column conflict notification.  |
| q  | This command comes from the coordinator (in response to the user command q). The block process writes a message (like Bye...) below the block, waits for some time (so the reader can read it), and exits.  |


Read the manual page for xterm to know what command-line options it supports. A window of default size is usually too big to store only a3×3block. You can set the window size and position to your desired values by the command-line option -geometry. You can specify a customized title by -T. A font-face (like Monospace)can be specified by -fa, and a font size by -fs. At the end, you specify, using -e, that you want to run ./block (instead of a default shell like bash). Here is an example.

xterm -T "Block 0" -fa Monospace -fs 15 -geometry "17x8+800+300" -bg "#331100" \

-e ./block blockno bfdin bfdout rn1fdout rn2fdout cn1fdout cn2fdout

Each block (that is, child) process exec’s this command to open and to transfer the display to an xterm. Set the geometries of the nine xterm’s appropriately, so that these windows line up as in a real Foodoku puzzle.Avoid manual relocation of the xterm’s by the user.

You may use the following makefile. Notice that boardgen.c is not to be compiled. It has no main function. It consists only of an implementation of the function newboard(). Only #include this in coordinator.c.

all: boardgen.c block.c coordinator.c

gcc -Wall -o block block.c

gcc -Wall -o coordinator coordinator.c

run: all

./coordinator

clean:

-rm -f block coordinator

Submit a single archive consisting of coordinator.c (your code), block.c (your code), blockgen.c (exactly as it is supplied to you), and makefile.

# Sample Output

## Run the coordinator


| Blocko  |
| -- |
| Block θ ready  |



| Block 1  |  |
| -- | -- |
| Block 1 ready  | Block 1 ready  |



| Block2  |  |
| -- | -- |
| Block 2  | ready  |


<!-- Block4 Block 4 ready D  -->
![](https://web-api.textin.com/ocr_image/external/f2dd69ec7201ec17.jpg)


| Bk3  |  |
| -- | -- |
| Block 3  | ready  |



| Dlocks  | Dlocks  |
| -- | -- |
| Block 5 ready  | Block 5 ready  |
|  |  |



| Tel = | Tel = |
| -- | -- |
| c315/Le.cundat cordatr /cedtorCrted  | c315/Le.cundat cordatr /cedtorCrted  |
| PDCO .  | Start now gane  |
| PDCO .  | Ft cell c[ ]of Bck () Prt s hl Wi  |
| Sbering ce for Macka and cells ·····111-·····--1314151 0  | Sbering ce for Macka and cells ·····111-·····--1314151 0  |



| Hok7  |  |
| -- | -- |
| Block 7 0  | ready  |



| Dlocke  |
| -- |
| Block 6 ready 0  |



| Dlck  |  |
| -- | -- |
| Block 8  | ready  |


## New game

<!-- Blocko Block1 Blockz 7 Blocks Block4 口 Blocks 1 Bocke DA7 Bocke 7 3  -->
![](https://web-api.textin.com/ocr_image/external/bff910aebb8f69b4.jpg)

<!-- 日 [cssrLe.cum bloch bo ardinetar.s dlnator codtsported pbcd Start n ga : ut dtgte & [1-3] at celt c[d-8] of Sck [6-6] Shew uan Prut tis hela nessaje Dil Mobering ac fe and clla 1314151 I6ITIEI Fo dPA CL  -->
![](https://web-api.textin.com/ocr_image/external/b1b6d6853cb6c984.jpg)


![](https://web-api.textin.com/ocr_image/external/c3629d7739e43f6c.jpg)

<!-- Blocko Block 1 Blockz x 9 Block3 Blecka Blocks 1 T Blocka 7 1 3  -->
![](https://web-api.textin.com/ocr_image/external/4af8813468aa88d5.jpg)

<!-- 日 Terninal s/////Lun -all-e atock.c Ell crdtercoerdatar.c cowd 1ater drted pbcd Lat Put dlgte d [1-9] st celt c[0-a] of biack [8-6] Shw sltn Prut uis hela nessge Di Hobering af and clLE .. I 11121 1314151 I6ITIEI dUe du 207z od  -->
![](https://web-api.textin.com/ocr_image/external/55c2aaff4ef7068d.jpg)

Replace a digit in a filled cell


| Block2  |
| -- |
|  |
|  |
|  |
|  |
|  |
|  |
|  |


<!-- Blocko Mock1 口 Black3 Block 4 1 Blocks Block 7 Block 71 1 3  -->
![](https://web-api.textin.com/ocr_image/external/86e573dac756be45.jpg)


| Blocks  |
| -- |
|  |
|  |
|  |
|  |
|  |
|  |



![](https://web-api.textin.com/ocr_image/external/76b8458247124ec2.jpg)


| 日 Te  | = |  |  |
| -- | -- | -- | -- |
| [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  | [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  | [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  |  |
| [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  | [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  | [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  |  |
| [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  | [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  | [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  |  |
| [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  | [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  | [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  |  |
| [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  | [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  | [CSE115]/ho/abhLj/IETKCPcoure/thec /LAste run mrdineter. r Su rted tet DDCO Put dg-9at cell c the slutn , Prt mis helPessage QAL Mobering acha for bld -111:,131 6ITIEI 11 00678 E  |  |



![](https://web-api.textin.com/ocr_image/external/d1297d5603df9ba3.jpg)


![](https://web-api.textin.com/ocr_image/external/4ccb628327dee9ad.jpg)

<!-- Blocko Block1 Blockz x 9 61714 Blocks Blecka Blocks 1 Readonly celll T Blocka ^  -->
![](https://web-api.textin.com/ocr_image/external/0747017d50dbd71e.jpg)

<!-- 日 Cunihal = s Mak pcc-wall-e coordiater coerdinater.c dutor Colnd wupgarted PbEd Start gane Put digit d [1-9]at cell c [a-a] of black b [8-6] Show salution Prist mi l ot Mbering sclere fer bladks and cells ToTET 1314151 -- 1617101 0Ue 06 Foodkue 0 5 5 5 au  -->
![](https://web-api.textin.com/ocr_image/external/734115db50caa471.jpg)


![](https://web-api.textin.com/ocr_image/external/ed944a106addebe1.jpg)


![](https://web-api.textin.com/ocr_image/external/011539ba116c7911.jpg)

<!-- D*1  -->
![](https://web-api.textin.com/ocr_image/external/0098e044d7f0b09d.jpg)

<!-- Block7 0  -->
![](https://web-api.textin.com/ocr_image/external/ac28af0bdd298d7d.jpg)

<!-- Black4 1 6  -->
![](https://web-api.textin.com/ocr_image/external/7b3701fcde7249ec.jpg)


| Block5  |
| -- |
|  |
| 7  |
|  |
| Block conflict  |


<!-- Blocko Block3 x 8 Doke HAKT Dake V 8 3  -->
![](https://web-api.textin.com/ocr_image/external/0ea3c55c9dd64068.jpg)


| Teninal = |
| -- |
| e at rdter /0rdnator |
| Coandsported Start no gane PDco uo & -) Shi M tian Prts hela s Dvit sber foe blls -·.1911121 ···1314151 161751  |
| 00d  |
| cod 672  |
| sod0678  |
| 11  |
| 20511  |



![](https://web-api.textin.com/ocr_image/external/9f30f2a666dac7a1.jpg)


![](https://web-api.textin.com/ocr_image/external/0ed06df654547224.jpg)

<!-- 立专种aN 家9话 三专. 中5． 家. -- ¥ · 一一1I站 1 中Binek n cellLn 中内临 漫rt2 CI P窖学 N公星 S三tuan 茶生览行 MM宝7 ［FM t e-生1 TeHniN P 5 B 3 7 1 B1 7 9 口 口 吴 Column contlzed 明 BkN  -->
![](https://web-api.textin.com/ocr_image/external/0199fdf997958c16.jpg)


| 爱厦o 田重家5 家专业5 . 6 ．一1 一专正酒0 -3 i 行I 对所7 专十7 A 二线 -I I 0 S 2 灵一-1 学A P S Ci 9 t 综M E   览2 E C e 佳车等l I 5 里S N . e e 】I  e l ·-A B A [n 5 F3 1 x u 1 6 9 口R 口O N  C P B o n t l z e U 9 7  |
| -- |


<!-- Blocka 514 8 71116 121319  -->
![](https://web-api.textin.com/ocr_image/external/b50e68da98bce652.jpg)

<!-- Blackz x 31 7 +···+. 214 9 5|8 1  -->
![](https://web-api.textin.com/ocr_image/external/eedc2f9f09664e15.jpg)


| Block1  |
| -- |
|  |
| 2  |
| 5 3 8 6 &lt; 4 <img src="https://web-api.textin.com/ocr_image/external/c924d8fde2b3c2ee.jpg"> |


<!-- Block3 6 2 8 1  -->
![](https://web-api.textin.com/ocr_image/external/dfeb54885d17503e.jpg)

<!-- Blacks 9 8 6 2 3 0  -->
![](https://web-api.textin.com/ocr_image/external/42f42cc71c389404.jpg)


| Bleck a 口 |
| -- |
|  |
|  |
|  |
| 4 2 9  |
| 8 7  |
|  |



![](https://web-api.textin.com/ocr_image/external/8bf9202185491373.jpg)


| 日 Crninal = |
| -- |
| s eparted Stert negane  |
| PDEd Put igi [1-9[3:9] |
| Shn Tan Print ia hl QVE  |
| bertng scere fer s and cells -1811121 11 , ··-.--1617181 ·. |
| codkus  |
| codkus p67 闪U555  |
| d 11  |
| dour 0586  |
|  |
| D  |


<!-- DE 7  -->
![](https://web-api.textin.com/ocr_image/external/540fecf0de1f99d6.jpg)


| D  |
| -- |
|  |
|  |


<!-- T 7 5  -->
![](https://web-api.textin.com/ocr_image/external/d74fa4389423e43f.jpg)

After this (for that matter, at any point of time, even during anongoing game), the user may start a new game by supplying the command n again.

## End game

<!-- Bok 5 2131 9 Bye.  -->
![](https://web-api.textin.com/ocr_image/external/25303fe254f9ea4e.jpg)

<!-- Blazkz x 31 7 21 9 518 1 Bye.  -->
![](https://web-api.textin.com/ocr_image/external/407e84cfc7ac20c4.jpg)


![](https://web-api.textin.com/ocr_image/external/3e1290fb481f08bd.jpg)


| Bock1  |
| -- |
|  |
| 2 1  |
|  |
| 5 8  |
|  |
| 6 7  |
|  |
| Bye...  |


<!-- Block3 6 2 8 17 3 915 1 Bye...l  -->
![](https://web-api.textin.com/ocr_image/external/56165e7fe42dcaec.jpg)

<!-- Blecks 9171 8 +*·******* 61115 4121 31 +··* Bye...  -->
![](https://web-api.textin.com/ocr_image/external/fd80b0ff798b589a.jpg)

<!-- Blocka 4| 2 9 B| 6 7 Bye...  -->
![](https://web-api.textin.com/ocr_image/external/66c72efb9eb252bb.jpg)


| 日 | Terminal = |  |  |
| -- | -- | -- | -- |
| bcd  | Sen g [1-9] at cell c [- of black 5 [-)Shi 1 L46 Print this hela nesse Duie  |  |  |
| Moder schelon feb · 121 .... ..·1--1 -··· | leckand cells  |  |  |
| F Colu.1A sodu -2. Rod ·:cod: |  |  |  |


<!-- Block 81 3 6 *** 719 12 11514 +···*··*** Bye..  -->
![](https://web-api.textin.com/ocr_image/external/94a8ad7484728ac2.jpg)

<!-- Bocke 41 9 2 *···*······- 118 5 1316|7  -->
![](https://web-api.textin.com/ocr_image/external/464543b740222b63.jpg)

<!-- Dk7 기 1 5 +···+·· 3 4 6 9 8 2 Bye.  -->
![](https://web-api.textin.com/ocr_image/external/12b20ecccf962c31.jpg)



