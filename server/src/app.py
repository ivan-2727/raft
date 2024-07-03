from flask import Flask, request
import time

app = Flask(__name__)

@app.route('/', methods = ['POST'])
def hello_world():
    return request.data

# main driver function
if __name__ == '__main__':
    app.run()