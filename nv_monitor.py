# import pynvml
# import time
# import json
# import requests  # For HTTP method

# pynvml.nvmlInit()
# handle = pynvml.nvmlDeviceGetHandleByIndex(0)
# info = pynvml.nvmlDeviceGetMemoryInfo(handle)
# print("Total memory:", info.total)
# print("Free memory:", info.free)
# print("Used memory:", info.used)
# print("Temperature is %d C" % pynvml.nvmlDeviceGetTemperature(handle, 0))
# print("Power is %d W" % pynvml.nvmlDeviceGetPowerUsage(handle))
# print("Utilization is %d %%" % pynvml.nvmlDeviceGetUtilizationRates(handle).gpu)
# pynvml.nvmlShutdown()

# def get_gpu_stats():
#     pynvml.nvmlInit()
#     handle = pynvml.nvmlDeviceGetHandleByIndex(0)
#     info = pynvml.nvmlDeviceGetMemoryInfo(handle)
#     total = info.total
#     free = info.free
#     used = info.used
#     temperature = pynvml.nvmlDeviceGetTemperature(handle, 0)
#     power = pynvml.nvmlDeviceGetPowerUsage(handle)
#     utilization = pynvml.nvmlDeviceGetUtilizationRates(handle).gpu
#     pynvml.nvmlShutdown()
#     stats = {
#         "total": total,
#         "free": free,
#         "used": used,
#         "temperature": temperature,
#         "power": power,
#         "utilization": utilization
#     }
#     return stats

# def send_via_http(stats):
#     """Send GPU stats to ESP32 via HTTP POST."""
#     try:
#         headers = {"Content-Type": "application/json"}
#         response = requests.post(ESP32_URL, data=json.dumps(stats), headers=headers)
#         print(f"HTTP Response: {response.status_code}, {response.text}")
#     except Exception as e:
#         print(f"HTTP Error: {e}")

# def main():
#     """Main loop to fetch and send data."""
#     while True:
#         stats = get_gpu_stats()
#         print(stats)

#         # Uncomment one of the following based on your preferred method
#         send_via_http(stats)  # Send via HTTP
#         # send_via_mqtt(stats)  # Send via MQTT

#         time.sleep(5)  # Adjust as needed


# if __name__ == "__main__":
#     try:
#         main()
#     except KeyboardInterrupt:
#         print("Stopping script...")
#         pynvml.nvmlShutdown()

from flask import Flask, jsonify
import pynvml

# Initialize NVML for NVIDIA GPU stats
pynvml.nvmlInit()

app = Flask(__name__)

def get_gpu_stats():
    """Fetches GPU stats from NVIDIA GPU."""
    handle = pynvml.nvmlDeviceGetHandleByIndex(0)  # Use first GPU
    temp = pynvml.nvmlDeviceGetTemperature(handle, pynvml.NVML_TEMPERATURE_GPU)
    mem_info = pynvml.nvmlDeviceGetMemoryInfo(handle)
    util = pynvml.nvmlDeviceGetUtilizationRates(handle)
    model = pynvml.nvmlDeviceGetName(handle)

    stats = {
        "temperature": temp,
        "memory_used": mem_info.used / (1024 * 1024 * 1024),
        "memory_total": mem_info.total / (1024 * 1024 * 1024),
        "gpu_usage": util.gpu,
        "gpu_model": "RTX 3060:"
    }
    return stats

@app.route('/gpu-stats', methods=['GET'])
def gpu_stats():
    """Handles GET request from ESP32 and returns GPU stats."""
    return jsonify(get_gpu_stats())

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)  # Change port if needed
