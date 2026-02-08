#!/bin/bash
# Agri-Wand Quick Setup Script
# This script sets up the development environment and starts the server

set -e  # Exit on error

echo "🌱 Agri-Wand Setup Script"
echo "=========================="
echo ""

# Check Python version
echo "📋 Checking Python version..."
python3 --version

# Create virtual environment if it doesn't exist
if [ ! -d "venv" ]; then
    echo "📦 Creating virtual environment..."
    python3 -m venv venv
else
    echo "✅ Virtual environment already exists"
fi

# Activate virtual environment
echo "🔧 Activating virtual environment..."
source venv/bin/activate

# Install dependencies
echo "📥 Installing dependencies..."
pip install --upgrade pip --quiet
pip install -r requirements.txt --quiet

echo ""
echo "✅ Setup complete!"
echo ""
echo "📊 Installed packages:"
pip list | grep -E "flask|opencv|google-generativeai|gunicorn|Pillow|numpy"

echo ""
echo "🚀 To start the server:"
echo "   source venv/bin/activate"
echo "   python3 fixed_ultra_server.py"
echo ""
echo "☁️  For production deployment:"
echo "   gunicorn fixed_ultra_server:app --bind 0.0.0.0:5000"
echo ""