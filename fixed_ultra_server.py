#!/usr/bin/env python3
"""
Fixed Ultra-Accurate Global Agricultural AI Wand
Enhanced tomato mold detection + ALL plant support
"""

from flask import Flask, request, jsonify, render_template_string
from flask_cors import CORS
import numpy as np
from PIL import Image, ImageEnhance, ImageOps, ImageFilter
import io
import os
import logging
from datetime import datetime
import socket
import json
import dotenv

# Load environment variables for local development
dotenv.load_dotenv()

try:
    import cv2
    CV2_AVAILABLE = True
except ImportError:
    CV2_AVAILABLE = False

try:
    import google.generativeai as genai
    GEMINI_AVAILABLE = True
except ImportError:
    GEMINI_AVAILABLE = False

app = Flask(__name__)
CORS(app)

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# Configuration
IMAGE_SIZE = (256, 256)
CONFIDENCE_THRESHOLD = 0.75
GEMINI_API_KEY = os.getenv('GEMINI_API_KEY', '')

UPLOAD_FOLDER = 'uploads'
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

# Initialize Gemini
gemini_model = None
if GEMINI_AVAILABLE and GEMINI_API_KEY:
    try:
        genai.configure(api_key=GEMINI_API_KEY)
        gemini_model = genai.GenerativeModel('gemini-1.5-flash')
        logger.info("✓ Gemini API initialized")
    except Exception as e:
        logger.warning(f"Gemini failed: {e}")

# Comprehensive Plant Database
GLOBAL_PLANTS = {
    'lebanese_herbs': ['Parsley', 'Mint', 'Cilantro', 'Dill', 'Thyme', 'Oregano', 'Rosemary', 'Sage', 'Basil'],
    'vegetables': ['Tomato', 'Cucumber', 'Eggplant', 'Zucchini', 'Cabbage', 'Cauliflower', 'Lettuce', 'Radish', 'Carrot', 'Onion', 'Garlic', 'Green beans'],
    'root_vegetables': ['Potato', 'Sweet potato', 'Carrot', 'Beetroot', 'Turnip', 'Parsnip', 'Ginger', 'Turmeric'],
    'leafy_greens': ['Spinach', 'Kale', 'Lettuce', 'Arugula', 'Swiss chard', 'Collard greens', 'Mustard greens', 'Watercress', 'Bok choy'],
    'fruit_vegetables': ['Tomato', 'Eggplant', 'Pepper', 'Cucumber', 'Zucchini', 'Squash', 'Pumpkin', 'Okra', 'Green beans', 'Pea', 'Corn'],
    'allium': ['Onion', 'Garlic', 'Shallot', 'Leek', 'Chive', 'Green onion'],
    'brassica': ['Cabbage', 'Broccoli', 'Cauliflower', 'Brussels sprouts', 'Kale', 'Collard greens', 'Kohlrabi'],
    'legumes': ['Green beans', 'Lima beans', 'Pea', 'Chickpea', 'Lentil', 'Soybean', 'Fava bean'],
    'herbs': ['Basil', 'Parsley', 'Cilantro', 'Mint', 'Dill', 'Rosemary', 'Thyme', 'Oregano', 'Sage', 'Chive'],
    'mediterranean': ['Olive', 'Fig', 'Pomegranate', 'Citrus', 'Almond', 'Walnut']
}

# Enhanced disease patterns for better detection
DISEASE_PATTERNS = {
    'tomato_leaf_mold': {
        'visual_pattern': 'yellow_upper_surface_gray_powder_underneath',
        'color_signature': {'yellow_upper': 0.15, 'gray_powder': 0.1, 'texture_var': 1000},
        'conditions': 'high_humidity_poor_ventilation',
        'treatment': 'increase_ventilation_reduce_humidity_copper_fungicide'
    },
    'tomato_early_blight': {
        'visual_pattern': 'concentric_rings_lower_leaves',
        'color_signature': {'brown_rings': 0.2, 'target_pattern': 0.15},
        'conditions': 'warm_humid_weather',
        'treatment': 'remove_affected_leaves_apply_fungicide'
    },
    'tomato_late_blight': {
        'visual_pattern': 'water_soaked_lesions_white_mold',
        'color_signature': {'water_soaked': 0.25, 'white_mold': 0.12},
        'conditions': 'cool_wet_conditions',
        'treatment': 'urgent_fungicide_application'
    },
    'powdery_mildew': {
        'visual_pattern': 'white_powder_surface',
        'color_signature': {'white_powder': 0.2, 'texture_smooth': 500},
        'conditions': 'moderate_temp_high_humidity',
        'treatment': 'sulfur_fungicide_improve_air_circulation'
    },
    'leaf_spot': {
        'visual_pattern': 'circular_brown_spots_halos',
        'color_signature': {'brown_spots': 0.18, 'yellow_halos': 0.1},
        'conditions': 'wet_humid_conditions',
        'treatment': 'copper_spray_remove_affected_leaves'
    }
}

class FixedUltraPlantAnalyzer:
    def __init__(self):
        self.gemini_available = gemini_model is not None
        self.cv2_available = CV2_AVAILABLE
    
    def advanced_preprocessing(self, image_bytes):
        """Enhanced preprocessing for better disease detection"""
        image = Image.open(io.BytesIO(image_bytes))
        original = image.copy()
        
        if image.mode != 'RGB':
            image = image.convert('RGB')
        
        # Multiple preprocessing stages
        processed_images = {}
        
        # Stage 1: Enhanced for disease detection
        img1 = ImageOps.autocontrast(image)
        img1 = ImageEnhance.Sharpness(img1).enhance(1.5)
        img1 = ImageEnhance.Contrast(img1).enhance(1.4)
        img1 = ImageEnhance.Color(img1).enhance(1.3)
        processed_images['enhanced'] = img1.resize(IMAGE_SIZE, Image.LANCZOS)
        
        # Stage 2: Color-focused for mold detection
        img2 = ImageEnhance.Color(image).enhance(1.5)
        img2 = ImageEnhance.Sharpness(img2).enhance(1.3)
        processed_images['color_enhanced'] = img2.resize(IMAGE_SIZE, Image.LANCZOS)
        
        # Stage 3: Texture-focused for spot detection
        img3 = image.filter(ImageFilter.DETAIL)
        img3 = ImageEnhance.Contrast(img3).enhance(1.5)
        processed_images['texture_enhanced'] = img3.resize(IMAGE_SIZE, Image.LANCZOS)
        
        return processed_images, original
    
    def detect_tomato_leaf_mold(self, img_array):
        """Specialized tomato leaf mold detection"""
        if not self.cv2_available:
            return None, 0.0
        
        try:
            # Convert to different color spaces
            hsv = cv2.cvtColor(img_array, cv2.COLOR_RGB2HSV)
            lab = cv2.cvtColor(img_array, cv2.COLOR_RGB2LAB)
            
            # Yellow upper surface detection
            yellow_upper_mask = (img_array[:, :, 0] > 150) & (img_array[:, :, 1] > 150) & (img_array[:, :, 2] < 100)
            yellow_upper_ratio = np.sum(yellow_upper_mask) / img_array.size
            
            # Gray/white powder underneath detection
            gray_powder_mask = (img_array[:, :, 0] > 180) & (img_array[:, :, 1] > 180) & (img_array[:, :, 2] > 180)
            gray_powder_ratio = np.sum(gray_powder_mask) / img_array.size
            
            # Texture analysis for powdery surface
            gray = cv2.cvtColor(img_array, cv2.COLOR_RGB2GRAY)
            texture_var = np.var(gray)
            
            # Enhanced detection logic
            mold_confidence = 0.0
            
            # Check for characteristic yellow upper surface
            if yellow_upper_ratio > 0.12:
                mold_confidence += 0.4
            
            # Check for grayish powder underneath
            if gray_powder_ratio > 0.08:
                mold_confidence += 0.3
            
            # Check texture variation (powdery surface)
            if texture_var > 800:
                mold_confidence += 0.3
            
            return 'Tomato - Leaf Mold', min(mold_confidence, 0.95)
            
        except Exception as e:
            logger.error(f"Tomato mold detection failed: {e}")
            return None, 0.0
    
    def detect_early_blight(self, img_array):
        """Detect early blight concentric rings"""
        if not self.cv2_available:
            return None, 0.0
        
        try:
            gray = cv2.cvtColor(img_array, cv2.COLOR_RGB2GRAY)
            
            # Look for circular patterns (concentric rings)
            circles = cv2.HoughCircles(
                gray, cv2.HOUGH_GRADIENT, dp=1, minDist=30,
                param1=50, param2=30, minRadius=5, maxRadius=25
            )
            
            if circles is not None:
                circles = np.round(circles[0, :]).astype("int")
                if len(circles) > 3:  # Multiple concentric circles
                    return 'Tomato - Early Blight', 0.8
            
            # Fallback to color analysis
            brown_mask = (img_array[:, :, 0] > 100) & (img_array[:, :, 1] < 100) & (img_array[:, :, 2] < 80)
            brown_ratio = np.sum(brown_mask) / img_array.size
            
            if brown_ratio > 0.15:
                return 'Tomato - Early Blight', 0.7
            
            return None, 0.0
            
        except Exception as e:
            logger.error(f"Early blight detection failed: {e}")
            return None, 0.0
    
    def detect_powdery_mildew(self, img_array):
        """Detect powdery mildew across plants"""
        if not self.cv2_available:
            return None, 0.0
        
        try:
            # White powder detection
            white_mask = (img_array[:, :, 0] > 200) & (img_array[:, :, 1] > 200) & (img_array[:, :, 2] > 200)
            white_ratio = np.sum(white_mask) / img_array.size
            
            # Texture analysis for powdery surface
            gray = cv2.cvtColor(img_array, cv2.COLOR_RGB2GRAY)
            
            # Powdery mildew has specific texture
            edges = cv2.Canny(gray, 50, 150)
            edge_density = np.sum(edges > 0) / edges.size
            
            if white_ratio > 0.15 and edge_density < 0.1:
                return 'Powdery Mildew', 0.85
            
            return None, 0.0
            
        except Exception as e:
            logger.error(f"Powdery mildew detection failed: {e}")
            return None, 0.0
    
    def general_plant_analysis(self, img_array):
        """General plant disease analysis"""
        # Color-based analysis
        avg_color = np.mean(img_array, axis=(0, 1))
        
        # Yellowing indicators
        if avg_color[1] > avg_color[0] and avg_color[1] > avg_color[2]:
            if avg_color[1] > 140:
                return 'Healthy Plant', 0.8
            else:
                return 'Nutrient Deficiency', 0.7
        
        # Browning indicators
        elif avg_color[0] > 120 and avg_color[1] < 100:
            return 'Fungal Infection', 0.75
        
        # General issues
        else:
            return 'Environmental Stress', 0.6
    
    def analyze_with_gemini_enhanced(self, image_bytes):
        """Enhanced Gemini analysis with better prompts"""
        if not self.gemini_available:
            return None
        
        try:
            image = Image.open(io.BytesIO(image_bytes))
            
            # Create comprehensive plant list
            all_plants = []
            for category, plants in GLOBAL_PLANTS.items():
                all_plants.extend(plants)
            
            prompt = f"""
            EXPERT PLANT PATHOLOGIST - Enhanced Analysis Required
            
            Analyze this plant image with precision:
            
            1. IDENTIFY PLANT from: {', '.join(all_plants[:20])}...
            
            2. DISEASE DETECTION - Look specifically for:
            - TOMATO LEAF MOLD: Yellow upper surface, grayish powder underneath
            - EARLY BLIGHT: Concentric rings on lower leaves  
            - LATE BLIGHT: Water-soaked lesions, white mold
            - POWDERY MILDEW: White powder on surface
            - LEAF SPOT: Circular lesions with halos
            
            3. VISUAL ANALYSIS:
            - Color patterns (yellowing, browning, spots)
            - Texture (powdery, smooth, rough)
            - Location on plant
            - Pattern distribution
            
            4. ENVIRONMENTAL FACTORS:
            - Humidity indicators
            - Temperature stress signs
            - Nutrient deficiency patterns
            
            CRITICAL: Be especially alert for TOMATO LEAF MOLD - it has very specific visual patterns!
            
            Respond with exact JSON:
            {{
                "plant_species": "exact plant name",
                "disease_name": "specific disease or 'healthy'",
                "confidence": 0.95,
                "visual_symptoms": "detailed visual description",
                "treatment": "specific treatment steps",
                "prevention": "prevention measures",
                "severity": "mild/moderate/severe"
            }}
            """
            
            response = gemini_model.generate_content([prompt, image])
            
            try:
                start = response.text.find('{')
                end = response.text.rfind('}') + 1
                if start != -1 and end != -1:
                    result = json.loads(response.text[start:end])
                    # Ensure confidence is reasonable
                    if result.get('confidence', 0) > 0.95:
                        result['confidence'] = 0.95
                    return result
            except:
                pass
            
            return None
            
        except Exception as e:
            logger.error(f"Enhanced Gemini analysis failed: {e}")
            return None
    
    def ultra_accurate_analysis(self, image_bytes):
        """Main analysis with enhanced tomato mold detection"""
        processed_images, original = self.advanced_preprocessing(image_bytes)
        
        # Use enhanced image for analysis
        img_array = np.array(processed_images['enhanced'])
        
        # Try specialized detection first
        disease, confidence = self.detect_tomato_leaf_mold(img_array)
        if disease and confidence > 0.7:
            return {
                'success': True,
                'disease': disease,
                'confidence': round(confidence * 100, 2),
                'is_healthy': False,
                'plant_type': 'Tomato',
                'treatment': 'Increase ventilation, reduce humidity, apply copper-based fungicide',
                'prevention': 'Ensure good air circulation, water at base of plants, avoid overhead watering',
                'severity': 'Moderate to Severe',
                'model_version': 'Enhanced Tomato Mold Detection',
                'detection_method': 'Specialized Algorithm',
                'timestamp': datetime.now().isoformat()
            }
        
        # Try early blight detection
        disease, confidence = self.detect_early_blight(img_array)
        if disease and confidence > 0.7:
            return {
                'success': True,
                'disease': disease,
                'confidence': round(confidence * 100, 2),
                'is_healthy': False,
                'plant_type': 'Tomato',
                'treatment': 'Remove affected leaves, apply fungicide, ensure proper spacing',
                'prevention': 'Crop rotation, resistant varieties, proper spacing',
                'severity': 'Moderate',
                'model_version': 'Enhanced Early Blight Detection',
                'detection_method': 'Specialized Algorithm',
                'timestamp': datetime.now().isoformat()
            }
        
        # Try powdery mildew detection
        disease, confidence = self.detect_powdery_mildew(img_array)
        if disease and confidence > 0.7:
            return {
                'success': True,
                'disease': disease,
                'confidence': round(confidence * 100, 2),
                'is_healthy': False,
                'plant_type': 'Unknown',
                'treatment': 'Apply sulfur fungicide, improve air circulation, remove affected leaves',
                'prevention': 'Proper spacing, good ventilation, resistant varieties',
                'severity': 'Moderate',
                'model_version': 'Enhanced Powdery Mildew Detection',
                'detection_method': 'Specialized Algorithm',
                'timestamp': datetime.now().isoformat()
            }
        
        # Try Gemini analysis
        gemini_result = self.analyze_with_gemini_enhanced(image_bytes)
        if gemini_result:
            return {
                'success': True,
                'disease': f"{gemini_result.get('plant_species', 'Unknown')} - {gemini_result.get('disease_name', 'Unknown')}",
                'confidence': round(gemini_result.get('confidence', 0.8) * 100, 2),
                'is_healthy': 'healthy' in gemini_result.get('disease_name', '').lower(),
                'plant_type': gemini_result.get('plant_species', 'Unknown'),
                'treatment': gemini_result.get('treatment', 'Monitor plant health'),
                'prevention': gemini_result.get('prevention', 'Good agricultural practices'),
                'severity': gemini_result.get('severity', 'Unknown'),
                'model_version': 'Enhanced Gemini AI',
                'detection_method': 'Google Gemini Vision',
                'timestamp': datetime.now().isoformat()
            }
        
        # Fallback to general analysis
        disease, confidence = self.general_plant_analysis(img_array)
        return {
            'success': True,
            'disease': disease,
            'confidence': round(confidence * 100, 2),
            'is_healthy': 'healthy' in disease.lower(),
            'plant_type': 'Unknown',
            'treatment': 'Monitor plant health and provide proper care',
            'prevention': 'Maintain good growing conditions',
            'severity': 'Unknown',
            'model_version': 'General Analysis',
            'detection_method': 'Color Analysis',
            'timestamp': datetime.now().isoformat()
        }

# Initialize analyzer
fixed_analyzer = FixedUltraPlantAnalyzer()

def fixed_predict_plant(image_bytes):
    """Main prediction function with enhanced accuracy"""
    return fixed_analyzer.ultra_accurate_analysis(image_bytes)

def enhance_with_sensors(ai_result, moisture, temp, humidity):
    """Enhance AI results with sensor context for better recommendations"""
    recommendations = []
    
    # Mold/Mildew + High Humidity
    if ('mold' in ai_result['disease'].lower() or 'mildew' in ai_result['disease'].lower()) and humidity > 75:
        recommendations.append({
            'priority': 'HIGH',
            'issue': f'High humidity ({humidity:.1f}%) promoting fungal growth',
            'current': f'{humidity:.1f}%',
            'optimal': 'Below 60%',
            'action': 'Improve ventilation immediately, reduce watering frequency'
        })
    
    # Any Disease + Low Soil Moisture
    if not ai_result['is_healthy'] and moisture < 25:
        recommendations.append({
            'priority': 'MEDIUM',
            'issue': f'Low soil moisture ({moisture:.1f}%) weakening plant immunity',
            'current': f'{moisture:.1f}%',
            'optimal': '40-60%',
            'action': 'Increase watering schedule while treating disease'
        })
    
    # Blight + High Temperature
    if 'blight' in ai_result['disease'].lower() and temp > 30:
        recommendations.append({
            'priority': 'MEDIUM',
            'issue': f'High temperature ({temp:.1f}°C) creating stress and promoting blight',
            'current': f'{temp:.1f}°C',
            'optimal': '20-28°C',
            'action': 'Provide shade, improve air circulation, water in early morning'
        })
    
    # Critical Environmental Conditions
    if moisture < 15:
        recommendations.append({
            'priority': 'CRITICAL',
            'issue': f'Extremely low soil moisture ({moisture:.1f}%)',
            'current': f'{moisture:.1f}%',
            'optimal': '40-60%',
            'action': 'Water immediately - plant is severely stressed'
        })
    
    if temp > 35:
        recommendations.append({
            'priority': 'CRITICAL',
            'issue': f'Dangerous temperature ({temp:.1f}°C) for most plants',
            'current': f'{temp:.1f}°C',
            'optimal': '20-28°C',
            'action': 'Move to shade immediately, provide cooling'
        })
    
    if humidity > 85:
        recommendations.append({
            'priority': 'HIGH',
            'issue': f'Very high humidity ({humidity:.1f}%) - high disease risk',
            'current': f'{humidity:.1f}%',
            'optimal': '50-70%',
            'action': 'Improve ventilation, reduce watering, increase spacing'
        })
    
    # Add sensor recommendations to result
    ai_result['sensor_recommendations'] = recommendations
    ai_result['sensor_data'] = {
        'soil_moisture': moisture,
        'temperature': temp,
        'humidity': humidity
    }
    
    logger.info(f"Added {len(recommendations)} sensor-based recommendations")
    return ai_result

# Enhanced HTML Template
HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Fixed Ultra-Accurate Agricultural AI Wand 🎯</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Segoe UI', sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 25%, #f093fb 50%, #f5576c 75%, #4facfe 100%); min-height: 100vh; padding: 20px; }
        .container { max-width: 1200px; margin: 0 auto; }
        .header { text-align: center; color: white; margin-bottom: 30px; }
        .header h1 { font-size: 2.8em; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }
        .header .version { background: rgba(255,255,255,0.2); padding: 5px 15px; border-radius: 20px; display: inline-block; margin-top: 10px; }
        .fixed-badge { background: linear-gradient(135deg, #28a745 0%, #20c997 100%); color: white; padding: 3px 10px; border-radius: 12px; font-size: 0.8em; margin-left: 10px; }
        .main-content { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 20px; }
        @media (max-width: 968px) { .main-content { grid-template-columns: 1fr; } }
        .card { background: white; border-radius: 15px; padding: 30px; box-shadow: 0 10px 30px rgba(0,0,0,0.2); }
        .upload-area { border: 3px dashed #28a745; border-radius: 10px; padding: 50px 20px; text-align: center; cursor: pointer; transition: all 0.3s ease; background: #f0fff4; }
        .upload-area:hover { background: #e8f5e8; }
        .upload-icon { font-size: 4em; margin-bottom: 20px; color: #28a745; }
        .upload-text { font-size: 1.2em; color: #333; margin-bottom: 10px; }
        .upload-hint { font-size: 0.9em; color: #666; }
        #fileInput { display: none; }
        .preview-container { margin-top: 20px; text-align: center; }
        .preview-image { max-width: 100%; max-height: 300px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); margin-bottom: 15px; }
        .analyze-btn { background: linear-gradient(135deg, #28a745 0%, #20c997 100%); color: white; border: none; padding: 15px 40px; font-size: 1.1em; border-radius: 25px; cursor: pointer; transition: all 0.3s ease; }
        .analyze-btn:hover { transform: translateY(-2px); }
        .analyze-btn:disabled { background: #ccc; cursor: not-allowed; }
        .loading { display: none; text-align: center; padding: 20px; }
        .loading.active { display: block; }
        .spinner { border: 4px solid #f3f3f3; border-top: 4px solid #28a745; border-radius: 50%; width: 50px; height: 50px; animation: spin 1s linear infinite; margin: 0 auto 15px; }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
        .results { display: none; }
        .results.active { display: block; }
        .result-header { background: linear-gradient(135deg, #28a745 0%, #20c997 100%); color: white; padding: 20px; border-radius: 10px; margin-bottom: 20px; text-align: center; }
        .result-header.warning { background: linear-gradient(135deg, #ffc107 0%, #fd7e14 100%); }
        .result-header.danger { background: linear-gradient(135deg, #dc3545 0%, #c82333 100%); }
        .disease-name { font-size: 1.8em; font-weight: bold; margin-bottom: 10px; }
        .confidence { font-size: 1.3em; }
        .model-info { background: rgba(255,255,255,0.2); padding: 5px 10px; border-radius: 15px; display: inline-block; margin-top: 10px; font-size: 0.9em; }
        .method-badge { background: rgba(255,255,255,0.2); padding: 3px 10px; border-radius: 12px; font-size: 0.8em; margin-left: 5px; }
        .treatment-section { background: #f8f9ff; padding: 20px; border-radius: 10px; margin-top: 15px; }
        .treatment-section h3 { color: #28a745; margin-bottom: 10px; }
        .detection-info { background: #d4edda; padding: 15px; border-radius: 10px; margin-top: 15px; border-left: 4px solid #28a745; }
        .btn-clear { background: #6c757d; color: white; border: none; padding: 10px 25px; border-radius: 20px; cursor: pointer; margin-top: 15px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🎯 Fixed Ultra-Accurate AI Wand</h1>
            <p>Enhanced Tomato Mold Detection + All Plant Support</p>
            <div class="version">Fixed Accuracy • Global Coverage <span class="fixed-badge">ENHANCED</span></div>
        </div>

        <div class="main-content">
            <div class="card">
                <h2 style="margin-bottom: 20px; color: #333;">Upload Plant Image</h2>
                
                <div class="upload-area" id="uploadArea">
                    <div class="upload-icon">📸</div>
                    <div class="upload-text">Drag & Drop Plant Image Here</div>
                    <div class="upload-hint">Fixed tomato mold detection + all plants supported</div>
                </div>
                
                <input type="file" id="fileInput" accept="image/*">
                
                <div class="preview-container" id="previewContainer" style="display: none;">
                    <img id="previewImage" class="preview-image" alt="Preview">
                    <br>
                    <button class="analyze-btn" id="analyzeBtn">🎯 Fixed Analysis</button>
                    <button class="btn-clear" id="clearBtn">✖ Clear</button>
                </div>
            </div>

            <div class="card">
                <h2 style="margin-bottom: 20px; color: #333;">Fixed Analysis Results</h2>
                
                <div class="loading" id="loading">
                    <div class="spinner"></div>
                    <p>Running enhanced analysis...</p>
                </div>

                <div class="results" id="results">
                    <div class="result-header" id="resultHeader">
                        <div class="disease-name" id="diseaseName">-</div>
                        <div class="confidence" id="confidence">-</div>
                        <div class="model-info" id="modelInfo">Model: -</div>
                        <div class="method-badge" id="methodBadge">Method: -</div>
                    </div>

                    <div class="treatment-section">
                        <h3>💊 Enhanced Treatment</h3>
                        <div id="treatment"></div>
                        <div id="prevention" style="margin-top: 10px;"></div>
                    </div>

                    <div class="detection-info">
                        <h4>🔍 Detection Details</h4>
                        <div id="detectionInfo"></div>
                    </div>
                </div>

                <div id="initialMessage" style="text-align: center; color: #999; padding: 50px 20px;">
                    <div style="font-size: 4em; margin-bottom: 20px;">🎯</div>
                    <p>Upload any plant image for fixed analysis</p>
                </div>
            </div>
        </div>
    </div>

    <script>
        let selectedFile = null;
        const uploadArea = document.getElementById('uploadArea');
        const fileInput = document.getElementById('fileInput');
        const previewContainer = document.getElementById('previewContainer');
        const previewImage = document.getElementById('previewImage');
        const analyzeBtn = document.getElementById('analyzeBtn');
        const clearBtn = document.getElementById('clearBtn');
        const loading = document.getElementById('loading');
        const results = document.getElementById('results');
        const initialMessage = document.getElementById('initialMessage');

        uploadArea.addEventListener('click', () => fileInput.click());
        
        uploadArea.addEventListener('dragover', (e) => {
            e.preventDefault();
            uploadArea.style.background = '#e8f5e8';
        });

        uploadArea.addEventListener('dragleave', () => {
            uploadArea.style.background = '#f0fff4';
        });

        uploadArea.addEventListener('drop', (e) => {
            e.preventDefault();
            uploadArea.style.background = '#f0fff4';
            const files = e.dataTransfer.files;
            if (files.length > 0) handleFile(files[0]);
        });

        fileInput.addEventListener('change', (e) => {
            if (e.target.files.length > 0) handleFile(e.target.files[0]);
        });

        function handleFile(file) {
            if (!file.type.startsWith('image/')) {
                alert('Please select an image file');
                return;
            }
            selectedFile = file;
            const reader = new FileReader();
            reader.onload = (e) => {
                previewImage.src = e.target.result;
                previewContainer.style.display = 'block';
            };
            reader.readAsDataURL(file);
        }

        clearBtn.addEventListener('click', () => {
            selectedFile = null;
            fileInput.value = '';
            previewContainer.style.display = 'none';
            results.classList.remove('active');
            initialMessage.style.display = 'block';
        });

        analyzeBtn.addEventListener('click', async () => {
            if (!selectedFile) {
                alert('Please select an image first');
                return;
            }

            loading.classList.add('active');
            results.classList.remove('active');
            initialMessage.style.display = 'none';
            analyzeBtn.disabled = true;

            try {
                const response = await fetch('/predict', {
                    method: 'POST',
                    body: selectedFile
                });

                const data = await response.json();
                loading.classList.remove('active');
                displayFixedResults(data);

            } catch (error) {
                loading.classList.remove('active');
                alert('Error: ' + error.message);
            } finally {
                analyzeBtn.disabled = false;
            }
        });

        function displayFixedResults(data) {
            const resultHeader = document.getElementById('resultHeader');
            const diseaseName = document.getElementById('diseaseName');
            const confidence = document.getElementById('confidence');
            const modelInfo = document.getElementById('modelInfo');
            const methodBadge = document.getElementById('methodBadge');
            const treatment = document.getElementById('treatment');
            const prevention = document.getElementById('prevention');
            const detectionInfo = document.getElementById('detectionInfo');

            resultHeader.className = 'result-header';
            if (!data.is_healthy) {
                if (data.confidence > 85) {
                    resultHeader.classList.add('danger');
                } else {
                    resultHeader.classList.add('warning');
                }
            }

            diseaseName.textContent = data.disease;
            confidence.textContent = `Confidence: ${data.confidence.toFixed(1)}%`;
            modelInfo.textContent = `Model: ${data.model_version}`;
            methodBadge.textContent = `Method: ${data.detection_method}`;

            treatment.innerHTML = `<strong>Treatment:</strong> ${data.treatment}`;
            prevention.innerHTML = `<strong>Prevention:</strong> ${data.prevention}`;

            detectionInfo.innerHTML = `
                <div><strong>Plant Type:</strong> ${data.plant_type}</div>
                <div><strong>Severity:</strong> ${data.severity}</div>
                <div><strong>Analysis Time:</strong> ${new Date(data.timestamp).toLocaleString()}</div>
            `;

            results.classList.add('active');
        }
    </script>
</body>
</html>"""

@app.route('/')
def index():
    hostname = socket.gethostname()
    local_ip = socket.gethostbyname(hostname)
    
    return render_template_string(
        HTML_TEMPLATE,
        server_url=f"http://{local_ip}:5000"
    )

@app.route('/predict', methods=['POST'])
def predict():
    try:
        # Get image data
        if request.files and 'file' in request.files:
            file = request.files['file']
            image_bytes = file.read()
        elif request.data:
            image_bytes = request.data
        else:
            return jsonify({'success': False, 'error': 'No image data'}), 400
        
        # Get sensor data from custom headers
        soil_moisture = float(request.headers.get('X-Soil-Moisture', 0))
        temperature = float(request.headers.get('X-Temperature', 0))
        humidity = float(request.headers.get('X-Humidity', 0))
        
        logger.info(f"Analysis request: {len(image_bytes)} bytes")
        if soil_moisture > 0 or temperature > 0 or humidity > 0:
            logger.info(f"Sensor data: Soil={soil_moisture:.1f}%, Temp={temperature:.1f}°C, Humidity={humidity:.1f}%")
        
        # Save image with timestamp
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        save_path = os.path.join(UPLOAD_FOLDER, f'analysis_{timestamp}.jpg')
        with open(save_path, 'wb') as f:
            f.write(image_bytes)
        
        # Perform AI analysis
        result = fixed_predict_plant(image_bytes)
        
        # Enhance with sensor context if sensor data provided
        if soil_moisture > 0 or temperature > 0 or humidity > 0:
            result = enhance_with_sensors(result, soil_moisture, temperature, humidity)
            logger.info(f"Enhanced result with {len(result.get('sensor_recommendations', []))} sensor recommendations")
        
        return jsonify(result), 200
    
    except Exception as e:
        logger.error(f"Prediction error: {str(e)}")
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/health', methods=['GET'])
def health():
    return jsonify({
        'status': 'healthy',
        'model_loaded': True,
        'model_version': 'Fixed Ultra-Accurate v5.0',
        'gemini_available': fixed_analyzer.gemini_available,
        'cv2_available': fixed_analyzer.cv2_available,
        'special_features': ['Enhanced Tomato Mold Detection', 'Early Blight Detection', 'Powdery Mildew Detection'],
        'plant_support': 'Global - All vegetables + Lebanese herbs',
        'accuracy_target': '90%+ for tomato diseases'
    }), 200

if __name__ == '__main__':
    logger.info("Starting Fixed Ultra-Accurate Agricultural AI Wand Server")
    logger.info(f"Gemini Available: {fixed_analyzer.gemini_available}")
    logger.info(f"OpenCV Available: {fixed_analyzer.cv2_available}")
    logger.info("Server starting on http://0.0.0.0:5000")
    app.run(host='0.0.0.0', port=5000, debug=False)
