
// 20 bytes containin the following data:
//| 0 | 1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 |
//| PM25  | PM10    | Latitude          | Longitude         | Year    | Mo | Da | Ho | Mi | Se | Ms |

function decodeUplink(input) {
  d = input.bytes;  
  pm25 = (d[0] << 8) | d[1];
  pm10 = (d[2] << 8) | d[3];

  latitude = byteArrayToFloat([d[4],d[5], d[6], d[7]]);
  longitude =  byteArrayToFloat([d[8],d[9], d[10], d[11]]);
  
  year = (d[12] << 8) | d[13];
  month = d[14];
  day = d[15];
  hour = d[16];
  minute = d[17];
  second = d[18];

  timestamp = new Date(Date.UTC(year, month, day, hour, minute, second)).toISOString();
  
  return {
    data: {
      bytes: toHexString(d),
      pm25: pm25,
      pm10: pm10,
      latitude: latitude,
      longitude: longitude,
      timestamp: timestamp
    },
    warnings: [],
    errors: []
  };
}

function toHexString(bytes) {
  return bytes.map(function(byte) {
    return (byte & 0xFF).toString(16)
  }).join('')
}

function byteArrayToFloat(byteArray) {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    
    for (let i = 0; i < 4; i++) {
        view.setUint8(i, byteArray[i]);
    }
    
    return view.getFloat32(0, false);
}