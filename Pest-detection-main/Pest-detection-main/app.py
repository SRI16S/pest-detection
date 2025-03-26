from flask import Flask, render_template, request, jsonify, send_file
import firebase_admin
from firebase_admin import credentials, storage
import numpy as np
from PIL import Image
import cv2
import os
import time
import logging
from ultralytics import YOLO
import io

app = Flask(__name__)

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# Initialize Firebase
cred = credentials.Certificate("iitm-1546f-firebase-adminsdk-n6v6w-d4e52824a9.json")
firebase_admin.initialize_app(cred, {
    'storageBucket': 'iitm-1546f.appspot.com'
})
bucket = storage.bucket()

# Load YOLO Model
model_path = "bit.pt"
if not os.path.exists(model_path):
    logging.error(f"Model file not found at: {model_path}")
    model = None
else:
    model = YOLO(model_path)

if model is None:
    logging.critical("Failed to load the model. The application cannot start.")

# Pest Classes and Treatments
PEST_CLASSES = {0: 'Bacterial-Spot', 1: 'Diseased', 2: 'TomatoRotten'}
TREATMENTS = {
    "Bacterial-Spot": ["Apply copper-based fungicides", "Remove infected leaves"],
    "Diseased": ["Consult with a plant pathologist", "Apply broad-spectrum fungicides"],
    "TomatoRotten": ["Remove affected fruit", "Improve drainage"],
    "Unknown": ["Consult with a plant expert"]
}

# Fetch image directly from Firebase Storage
def fetch_image_from_firebase(image_filename):
    try:
        blob = bucket.blob(f'images/{image_filename}')
        image_data = blob.download_as_bytes()
        img = Image.open(io.BytesIO(image_data)).convert('RGB')
        return img
    except Exception as e:
        logging.error(f"Error fetching image from Firebase: {e}")
        return None

# Process image for visualization
def process_image_for_display(img, results):
    try:
        img = np.array(img)
        img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)

        for box in results.boxes:
            xyxy = box.xyxy[0].numpy().astype(int)
            cls_id = int(box.cls[0])
            conf = float(box.conf[0])
            label = f"{PEST_CLASSES.get(cls_id, 'Unknown')} ({conf:.2f})"

            color = (0, 255, 0)
            cv2.rectangle(img, (xyxy[0], xyxy[1]), (xyxy[2], xyxy[3]), color, 2)
            cv2.putText(img, label, (xyxy[0], xyxy[1] - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)

        return Image.fromarray(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))
    except Exception as e:
        logging.error(f"Error processing image: {e}")
        return None

# --- Routes ---
@app.route('/')
def home():
    return render_template('sri1.html')

@app.route('/dashboard')
def dashboard():
    return render_template('new1.html')

@app.route('/pest-detection')
def pest_detection():
    return render_template('y1.html')

@app.route('/sensor-data')
def sensor_data():
    return render_template('final1.html')

# --- API Endpoint ---
@app.route('/api/fetch-image', methods=['POST'])
def analyze_firebase_image():
    try:
        data = request.get_json()
        if not data or 'image_filename' not in data:
            return jsonify({'error': 'Image filename is missing in the request'}), 400

        image_filename = data['image_filename']
        img = fetch_image_from_firebase(image_filename)
        if img is None:
            return jsonify({'error': 'Failed to retrieve image from Firebase'}), 500

        start_time = time.time()
        results = model.predict(img)[0]  # YOLOv8 inference

        # Get best prediction
        pest_name = "Unknown"
        confidence = 0
        treatment_steps = TREATMENTS["Unknown"]

        if results.boxes:
            best_box = results.boxes[0]
            predicted_class = int(best_box.cls[0])
            confidence = float(best_box.conf[0])
            pest_name = PEST_CLASSES.get(predicted_class, "Unknown")
            treatment_steps = TREATMENTS.get(pest_name, ["Consult with a plant expert"])

        # Process image with bounding boxes
        processed_img = process_image_for_display(img, results)
        if processed_img is None:
            return jsonify({'error': 'Error processing image'}), 500

        # Save processed image locally
        processed_image_path = f'static/processed_{image_filename}'
        processed_img.save(processed_image_path)

        processing_time = time.time() - start_time

        return jsonify({
            'success': True,
            'image_filename': image_filename,
            'prediction': pest_name,
            'confidence': round(confidence, 2),
            'treatment': treatment_steps,
            'processed_image_url': f'/static/processed_{image_filename}',
            'processing_time': round(processing_time, 2)
        })

    except Exception as e:
        logging.exception(f'Error processing image: {str(e)}')
        return jsonify({'error': f'Error processing image: {str(e)}'}), 500

# Serve processed image
@app.route('/processed-image/<filename>')
def serve_processed_image(filename):
    return send_file(f'static/{filename}', mimetype='image/jpeg')

if __name__ == '__main__':
    app.run(debug=True)
