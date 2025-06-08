
// 20 bytes containing the following data:
//| 0 | 1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 |
//| PM25  | PM10    | Latitude          | Longitude         | YY      | M  | D  | H  | m  | S |

function decodeUplink(input) {
  d = input.bytes;  
  pm25 = (d[0] << 8) | d[1];
  pm10 = (d[2] << 8) | d[3];

  latitude = byteArrayToFloat([d[4],d[5], d[6], d[7]]);
  longitude =  byteArrayToFloat([d[8],d[9], d[10], d[11]]);
  altitude = d[12];
  
  year = (d[13] << 8) | d[14];
  month = d[15];
  day = d[16];
  hour = d[17];
  minute = d[18];
  second = d[19];
  
  timestamp = new Date(Date.UTC(year, month-1, day, hour, minute, second)).toISOString();
  
  return {
    data: {
      altitude: altitude,
      latitude: latitude,
      longitude: longitude,
      bytes: toHexString(d),
      pm10: pm10,
      pm25: pm25,
      timestamp: timestamp
    },
    warnings: [],
    errors: []
  };
}

function toHexString(bytes) {
  return bytes.map(function(byte) {
    return (byte & 0xFF).toString(16).padStart(2,0)
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