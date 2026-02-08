import os
import google.generativeai as genai
from dotenv import load_dotenv

load_dotenv()
api_key = os.getenv('GEMINI_API_KEY')

print(f"Testing API Key: {api_key[:5]}...{api_key[-5:] if api_key else 'None'}")

genai.configure(api_key=api_key)
try:
    model = genai.GenerativeModel('gemini-1.5-flash')
    response = model.generate_content("Say 'Gemini is working!'")
    print(f"Response: {response.text}")
except Exception as e:
    print(f"Error: {e}")
