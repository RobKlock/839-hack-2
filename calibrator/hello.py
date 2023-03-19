from flask import Flask, jsonify, render_template, request
app = Flask(__name__)

# Shamelessly copied from https://github.com/pallets/flask/tree/2.0.3/examples/javascript

sonar_data = []
sensor_data = []

@app.route("/")
def index():
    return render_template(f"calibrator.html")


@app.route("/calibrate", methods=["POST"])
def calibrate():
    sonar = request.form.get("sonar", 0, type=float)
    bed_sensor = request.form.get("bed_sensor", False, type=bool)
    sonar_data.append(sonar)
    sensor_data.append(bed_sensor)
    print({"sonar": sonar_data, "bed_sensor": sensor_data})
    return jsonify(sonar_data=str(sonar_data), sensor_data=str(sensor_data))