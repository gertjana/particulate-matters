# Data explaination

```hex
004C006C425129E4409ACF9907E9051D073A2C00
```

a data package is a 20 byte array containing the following

All values are Little Endian

| Meaning   | Data      | Type    | Value      |
| --------- | --------- | ------- | ---------- |
| PM 2.5 uM | 00C4      | INT16   | 76         |
| PM 10 uM  | 006C      | INT16   | 108        |
| Latitude  | 425129E4  | FLOAT32 | 52.29091   |
| Longitude | 409ACF99  | FLOAT32 | 4.83784151 |
| Year      | 07E9      | INT16   | 2025       |
| Month     | 05        | INT8    | 5          |
| Day       | 1D        | INT8    | 29         |
| Hour      | 07        | INT8    | 7          |
| Minute    | 3A        | INT8    | 58         |  
| Second    | 2C        | INT8    | 44         |
| CentiSec  | 00        | INT8    | 00         |