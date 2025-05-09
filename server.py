from flask import Flask
from flask import request

app = Flask(__name__)

currentLat = None

@app.route("/track")
def receive_data():
    global currentLat
    offset = 0.000025

    if currentLat == None:
        return "Not calibrated"

    lat = float(request.args.get("lat"))
    lng = float(request.args.get("lng"))

    print(f'Received lat: {lat}')
    print(f'Received long: {lng}')

    if abs(lat - currentLat) < offset:
        print("IN BOUNDS")
        return "IN-BOUNDS"
    else:
        print("OUT OF BOUNDS")
        return "OOB"

@app.route("/init")
def calibrate():
    global currentLat
    lat_str = request.args.get("lat")
    currentLat = float(lat_str)

    print(f'Initialized latitude: {currentLat}')
    return "Coordinates initialized"

