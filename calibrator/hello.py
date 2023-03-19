from flask import Flask, request
app = Flask(__name__)

@app.route("/test", methods=["POST"])
def test():
    json = request.get_json()
    sonar = json['sonar']
    button = json['bed_sensor']
    return f"Sonar: {sonar}, Button: {button}\n"

if __name__ == '__main__':
   app.run(debug = True)