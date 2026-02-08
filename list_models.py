import google.generativeai as genai
api_key = "AIzaSyDmk2Lxba6qbYKL7Z6-vWUTWSEJpV087bQ"
genai.configure(api_key=api_key)
try:
    print("Listing models...")
    for m in genai.list_models():
        if 'generateContent' in m.supported_generation_methods:
            print(f"- {m.name}")
except Exception as e:
    print(f"Error listing models: {e}")
