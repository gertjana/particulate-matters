import json
import folium
from folium import plugins
from datetime import datetime
from collections import defaultdict

def load_data(filename):
    with open(filename, 'r') as file:
        data = json.load(file)
    return data

def extract_coordinates_and_measurements(data):
    points_pm10 = []
    points_pm25 = []
    for message in data:
        # Get coordinates from the locations.frm-payload
        location = message['uplink_message'].get('locations', {}).get('frm-payload', {})
        payload = message['uplink_message']['decoded_payload']
        
        lat = location.get('latitude')
        lon = location.get('longitude')
        
        if lat and lon:  # Only include points with valid coordinates
            # Create points with [lat, lon, measurement]
            points_pm10.append([lat, lon, payload['pm10']])
            points_pm25.append([lat, lon, payload['pm25']])
    
    return points_pm10, points_pm25

def create_heatmap(points, title):
    # Center the map on the average coordinates
    center_lat = sum(p[0] for p in points) / len(points)
    center_lon = sum(p[1] for p in points) / len(points)
    
    # Create base map
    m = folium.Map(location=[center_lat, center_lon], 
                   zoom_start=15,
                   tiles='OpenStreetMap')
    
    # Add title
    title_html = f'''
        <div style="position: fixed; 
                    top: 10px; left: 50%;
                    transform: translateX(-50%);
                    background-color: white;
                    padding: 10px;
                    border-radius: 5px;
                    z-index: 1000;
                    font-size: 16px;
                    font-weight: bold;">
            {title}
        </div>
    '''
    m.get_root().html.add_child(folium.Element(title_html))
    
    # Calculate measurement ranges
    measurements = [p[2] for p in points]
    min_val = min(measurements)
    max_val = max(measurements)
    
    # Add heatmap layer
    heatmap = plugins.HeatMap(points,
                            min_opacity=0.4,
                            max_val=max_val,
                            radius=25, 
                            blur=15,
                            gradient={0.4: 'blue', 0.65: 'lime', 0.8: 'yellow', 1: 'red'})
    
    heatmap.add_to(m)
    
    # Group nearby points (less than 10 meters apart)
    proximity_threshold = 0.0001  # roughly 10 meters in decimal degrees
    point_clusters = defaultdict(list)
    
    for lat, lon, measurement in points:
        # Round coordinates to group nearby points
        cluster_key = (round(lat / proximity_threshold) * proximity_threshold, 
                       round(lon / proximity_threshold) * proximity_threshold)
        point_clusters[cluster_key].append((lat, lon, measurement))
    
    # Add markers for each measurement or cluster
    for cluster_key, cluster_points in point_clusters.items():
        avg_lat = sum(p[0] for p in cluster_points) / len(cluster_points)
        avg_lon = sum(p[1] for p in cluster_points) / len(cluster_points)
        
        if len(cluster_points) == 1:
            # Single point - just add a black dot
            folium.CircleMarker(
                location=[avg_lat, avg_lon],
                radius=3,
                color='black',
                fill=True,
                fill_color='black',
                fill_opacity=1
            ).add_to(m)
        else:
            # Multiple points close together - add a numbered marker
            folium.CircleMarker(
                location=[avg_lat, avg_lon],
                radius=10,
                color='black',
                fill=True,
                fill_color='white',
                fill_opacity=0.8,
                tooltip=f"{len(cluster_points)} measurements"
            ).add_to(m)
            
            # Add the number as text
            folium.map.Marker(
                [avg_lat, avg_lon],
                icon=folium.DivIcon(
                    icon_size=(20, 20),
                    icon_anchor=(10, 10),
                    html=f'<div style="font-size: 10pt; color: black; text-align: center; font-weight: bold;">{len(cluster_points)}</div>'
                )
            ).add_to(m)
    
    # Create legend
    legend_html = f'''
        <div style="position: fixed; 
                    bottom: 50px; right: 50px;
                    background-color: white;
                    padding: 10px;
                    border-radius: 5px;
                    z-index: 1000;
                    font-size: 14px;">
            <h4 style="margin-top: 0;">Measurements</h4>
            <div><i style="background: red; display: inline-block; width: 15px; height: 15px;"></i> {int(max_val * 0.8)} - {int(max_val)} μg/m³</div>
            <div><i style="background: yellow; display: inline-block; width: 15px; height: 15px;"></i> {int(max_val * 0.65)} - {int(max_val * 0.8)} μg/m³</div>
            <div><i style="background: lime; display: inline-block; width: 15px; height: 15px;"></i> {int(max_val * 0.4)} - {int(max_val * 0.65)} μg/m³</div>
            <div><i style="background: blue; display: inline-block; width: 15px; height: 15px;"></i> {int(min_val)} - {int(max_val * 0.4)} μg/m³</div>
            <div><i style="background: black; display: inline-block; width: 15px; height: 15px; border-radius: 50%;"></i> Measurement location</div>
            <div><i style="background: white; display: inline-block; width: 15px; height: 15px; border: 1px solid black; border-radius: 50%; text-align: center; font-size: 10px;">n</i> n measurements in same area</div>
            <hr style="margin: 5px 0;">
            <div>Range: {int(min_val)} - {int(max_val)} μg/m³</div>
            <div>Samples: {len(points)}</div>
        </div>
    '''
    m.get_root().html.add_child(folium.Element(legend_html))
    
    return m

def main():
    # Load and process data
    data = load_data('stored_messages.json')
    points_pm10, points_pm25 = extract_coordinates_and_measurements(data)
    
    # Create heatmaps
    heatmap_pm10 = create_heatmap(points_pm10, 'PM10 Concentration (μg/m³)')
    heatmap_pm25 = create_heatmap(points_pm25, 'PM2.5 Concentration (μg/m³)')
    
    # Save to HTML files
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    heatmap_pm10.save(f'heatmap_pm10_{timestamp}.html')
    heatmap_pm25.save(f'heatmap_pm25_{timestamp}.html')
    
    print(f"Heatmaps have been created:")
    print(f"- heatmap_pm10_{timestamp}.html")
    print(f"- heatmap_pm25_{timestamp}.html")

if __name__ == "__main__":
    main()
