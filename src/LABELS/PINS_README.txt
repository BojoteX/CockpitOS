S2 to S3 GPIO conversion table the PIN() macro does this automatically

Example: Simply use PIN(12) on an S2 and it will automatically convert it to 10 on an S3 (see table below)

Why is this useful? you can swap an S2 for an S3 transparently on any PCB from TEK creations 

-----------------------------------------------
| S2 Mini GPIO | S3 Mini GPIO (same position) |
| ------------ | ---------------------------- |
| 3            | 2                            |
| 5            | 4                            |
| 7            | 12                           |
| 9            | 13                           |
| 12           | 10                           |
| 2            | 3                            |
| 4            | 5                            |
| 8            | 7                            |
| 10           | 8                            |
| 13           | 9                            |
| 40           | 33                           |
| 38           | 37                           |
| 36           | 38                           |
| 39           | 43                           |
| 37           | 44                           |
| 35           | 36                           |
| 33           | 35                           |
-----------------_-----------------------------