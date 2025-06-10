# Data explaination

This is an example sent from the device:

```hex
004C006C425129E4409ACF990707E9051D073A2C
```

It's a 20 byte array containing the following data

All values are Little Endian

| Meaning       | Data      | Type    | Value      |
| ------------- | --------- | ------- | ---------- |
| PM 2.5 uM     | 00C4      | INT16   | 76         |
| PM 10 uM      | 006C      | INT16   | 108        |
| Latitude      | 425129E4  | FLOAT32 | 52.29091   |
| Longitude     | 409ACF99  | FLOAT32 | 4.83784151 |
| Altitude      | 07        | INT8    | 7          |
| Year          | 07E9      | INT16   | 2025       |
| Month         | 05        | INT8    | 5          |
| Day           | 1D        | INT8    | 29         |
| Hour          | 07        | INT8    | 7          |
| Minute        | 3A        | INT8    | 58         |  
| Second        | 2C        | INT8    | 44         |

After passing the 20 byte through the dataformatter defined in the TTN Console, the output looks like this

```json
{
  "altitude": 7,
  "bytes": ,"004C006C425129E4409ACF990707E9051D073A2C"
  "latitude": 52.29091,
  "longitude": 4.83784151,
  "pm10": 108,
  "pm25": 76,
  "timestamp": "2025-05-29T07:58:44Z"
}
```
