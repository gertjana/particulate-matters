# Heatmap Visualization

This folder contains scripts to visualize particulate matter measurements on interactive heatmaps.

## Prerequisites
- Python 3.x installed
- pip (Python package installer)
- A JSON file with measurements from The Things Network (provided in `stored_messages_*.json`)

## Setup Instructions

1. Create and activate Python virtual environment:
```fish
# From the project root
python3 -m venv venv

# Activate the virtual environment (for fish shell)
source venv/bin/activate.fish
```

2. Install required packages:
```fish
pip install folium
```

## Generating Heatmaps

Make sure your virtual environment is activated (you should see `(venv)` in your prompt), then:

1. Navigate to this directory:
```fish
cd heatmap
```

2. Run the heatmap generator:
```fish
python create_heatmap.py
```

The script will generate two HTML files:
- `heatmap_pm10_[timestamp].html`: Heatmap showing PM10 concentrations
- `heatmap_pm25_[timestamp].html`: Heatmap showing PM2.5 concentrations

## Viewing the Heatmaps

Open the generated HTML files in a web browser. Each heatmap shows:
- Interactive map with color-coded concentrations
  - Blue: Low concentration
  - Green: Medium-low concentration
  - Yellow: Medium-high concentration
  - Red: High concentration
- Legend box showing:
  - Measurement ranges for each color
  - Total range of measurements
  - Number of samples
- Timestamp in the filename

You can:
- Zoom in/out of the map
- Pan around
- Hover over areas to see concentration intensity

## Managing the Virtual Environment

When you're done, deactivate the virtual environment:
```fish
deactivate
```

To reactivate it later:
```fish
source venv/bin/activate.fish
```

## Input Data Format

The script expects a JSON file (`stored_messages_*.json`) containing measurements from The Things Network with the following structure for each measurement:
```json
{
  "uplink_message": {
    "decoded_payload": {
      "pm10": <number>,
      "pm25": <number>
    },
    "locations": {
      "frm-payload": {
        "latitude": <number>,
        "longitude": <number>
      }
    }
  }
}
```

For more details about the data format and byte structure, see `../example.md`.
