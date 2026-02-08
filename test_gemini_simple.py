import google.generativeai as genai
api_key = "AIzaSyDmk2Lxba6qbYKL7Z6-vWUTWSEJpV087bQ"
print(f"Testing API Key: {api_key[:10]}...")
genai.configure(api_key=api_key)
try:
    model = genai.GenerativeModel('gemini-1.5-flash')
    response = model.generate_content("Hello! Testing connection.")
    print(f"SUCCESS: {response.text}")
except Exception as e:
    print(f"FAILED: {e}")
