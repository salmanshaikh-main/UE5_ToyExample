import flask
from flask import jsonify
import subprocess
import os

app = flask.Flask(__name__)

def get_active_interface():
    try:
        # Actual
        # result = subprocess.check_output("ip route | grep default | awk '{print $5}' | head -n1", shell=True).decode().strip()

        # Loopback for now
        result = subprocess.check_output("ip -o link show | awk '{print $2,$9}' | grep -w 'lo' | awk -F':' '{print $1}'", shell=True).decode().strip()

        print(f"Selected interface {result}")
        if not result:
            result = subprocess.check_output("ip link show | grep 'state UP' | awk -F': ' '{print $2}' | head -n1", shell=True).decode().strip()
        return result
    except Exception as e:
        print(f"Error getting active interface: {e}")
        return None

# Get the active interface when the app starts
active_interface = get_active_interface()

@app.route('/apply_delay', methods=['GET'])
def apply_delay():
    delay = 2000  # Default 2000ms
    duration = 10  # Default 10 seconds

    if not active_interface:
        return jsonify({"error": "No active network interface found"}), 400
    
    try:
        # Delete any existing traffic control rules on the interface
        os.system(f"tc qdisc del dev {active_interface} root")
        
        # Apply delay
        os.system(f"tc qdisc add dev {active_interface} root netem delay {delay}ms")
        
        # Schedule removal
        os.system(f"(sleep {duration} && tc qdisc del dev {active_interface} root) &")
        
        return jsonify({"message": "Delay applied successfully", "interface": active_interface}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(host='localhost', port=5000)
