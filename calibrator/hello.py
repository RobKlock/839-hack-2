from typing import List
from flask import Flask, jsonify, render_template, request
import requests
app = Flask(__name__)

# Shamelessly copied from https://github.com/pallets/flask/tree/2.0.3/examples/javascript

# Store sensor data in arrays, for now.
sonar_data: List[float] = []
button_data: List[int] = []
labels: List[int] = []

# Calibration state
learning_complete: bool = False
samples_per_label: int = 10
current_label: bool = False
samples_remaining: int = samples_per_label

@app.route("/")
def index():
    return render_template(f"calibrator.html")


def learn():
    # TODO: Learn!
    pass


def add_sample():
    latest_data = requests.get("http://3.138.135.239:3000/data").json()[-1]
    if isinstance(latest_data["sonar"], list):
        sonar_data.append(latest_data["sonar"][-1])
        button_data.append(latest_data["bed_sensor"][-1])
    elif isinstance(latest_data["sonar"], float):
        sonar_data.append(latest_data["sonar"])
        button_data.append(latest_data["bed_sensor"])
    labels.append(current_label)
    

@app.route("/calibrate", methods=["POST"])
def calibrate():
    global current_label
    global samples_per_label
    global samples_remaining
    global learning_complete

    if learning_complete:
        return "{}"

    # First, grab another sample
    add_sample()
    samples_remaining -= 1

    # Then, update state
    if not current_label:
        if samples_remaining == 0:
            current_label = True
            samples_remaining = samples_per_label
            direction = "Now, lay on your bed. Once you are in a sleeping position, continue to press the button. For a robust calibration, adjust your position between button pushes."
            message = f"Press the button {samples_remaining} more times!"
        else:
            message = f"Press the button {samples_remaining} more times!"
    else:
        if samples_remaining == 0:
            learn()
            learning_complete = True
            direction = ""
            message = f"Learning Complete!"
        else:
            message = f"Press the button {samples_remaining} more times!"
    
    json_kwargs = dict(
        sonar_data = str(sonar_data),
        sensor_data = str(button_data),
        label_data = str(labels),
        message = message,
    )
    if "direction" in locals():
        json_kwargs["direction"] = direction

    return jsonify(**json_kwargs)