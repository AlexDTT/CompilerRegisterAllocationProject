# Description of datasets for the basic algorithm

| Range         | Registers        | Ideal allocation | Allocation        |
| ------------- | ---------------- | ---------------- | ----------------- |
| `ranges1.txt` | `registers2.txt` | 2 registers      | `allocation1.txt` |
| `ranges2.txt` | `registers2.txt` | 2 registers      | `allocation2.txt` |
| `ranges3.txt` | `registers2.txt` | 2 registers      | `allocation3.txt` |
| `ranges4.txt` | `registers1.txt` | 1 register       | `allocation4.txt` |
| `ranges5.txt` | `registers1.txt` | 1 register       | `allocation5.txt` |
| `ranges6.txt` | `registers3.txt` | 3 registers      | `allocation6.txt` |

## Output

The `tests/output/expected/basic` folder contains valid solutions produced by this project for
each input instance.

It is important to note that:
- The naming of webs can differ across implementations.
- The specific register chosen for a web can also differ.

Those differences are acceptable as long as the allocation is valid and respects the provided
register bound.
