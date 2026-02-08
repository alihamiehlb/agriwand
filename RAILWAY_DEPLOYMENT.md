# 🚀 Railway.app Deployment - Step by Step

## Why Railway.app?
- ✅ **Always on** (no sleep like Render)
- ✅ **$5 free credit/month** (enough for 500+ analyses)
- ✅ **Auto-deploy from GitHub** (push code → auto-updates)
- ✅ **Perfect for Python/Flask**
- ✅ **Fast** (3-5 second responses)

---

## Step 1: Sign Up (2 minutes)

1. Go to: https://railway.app
2. Click **"Start a New Project"** or **"Login"**
3. Choose **"Login with GitHub"**
4. Authorize Railway to access your repositories
5. ✅ Done!

---

## Step 2: Create New Project (3 minutes)

1. Click **"New Project"**
2. Select **"Deploy from GitHub repo"**
3. Choose **"alihamiehlb/agriwand"** from the list
4. Railway will:
   - Detect Python project ✅
   - Find `Procfile` ✅
   - Find `requirements.txt` ✅
   - Start building automatically ✅

---

## Step 3: Add Environment Variable (1 minute)

**IMPORTANT**: Add your Gemini API key

1. Click on your deployed project
2. Go to **"Variables"** tab (left sidebar)
3. Click **"New Variable"**
4. Add:
   - **Variable Name**: `GEMINI_API_KEY`
   - **Value**: Your Gemini API key (paste it here)
5. Click **"Add"**
6. Service will automatically redeploy with the new variable

---

## Step 4: Wait for Deployment (2-3 minutes)

Watch the deployment logs:
- **Building**: Installing dependencies
- **Deploying**: Starting server
- **Success**: ✅ Service is live!

---

## Step 5: Get Your URL

1. Click on your service
2. Go to **"Settings"** tab
3. Find **"Domains"** section
4. You'll see something like:
   ```
   https://agriwand-production.up.railway.app
   ```
5. **Copy this URL** - you'll need it for ESP32!

---

## Step 6: Test Your Deployment

Open terminal and test:

```bash
# Test health endpoint
curl https://YOUR-URL.up.railway.app/health
```

**Expected response**:
```json
{
  "status": "healthy",
  "gemini_available": true,
  "opencv_available": true,
  "server_time": "2026-02-08 20:00:00"
}
```

If you see this, **deployment successful!** ✅

---

## Step 7: Update ESP32 with Cloud URL

### Option A: Via WiFiManager (Easy)
1. Power on ESP32
2. If already configured, hold reset button during boot (if you added one)
3. Connect to "Agri-Wand-Setup" WiFi (password: `admin123`)
4. Browser opens automatically
5. Enter:
   - **WiFi Name**: Your WiFi
   - **WiFi Password**: Your password
   - **AI Server URL**: `https://YOUR-URL.up.railway.app/predict`
6. Click **"Save"**
7. Done!

### Option B: Via Code (If no WiFiManager yet)
1. Edit `agri_wand_with_button.ino`
2. Find line 59:
   ```cpp
   String serverURL = "http://192.168.1.14:5000/predict";
   ```
3. Change to:
   ```cpp
   String serverURL = "https://YOUR-URL.up.railway.app/predict";
   ```
4. Upload to ESP32-CAM

---

## Step 8: Test End-to-End

1. **Press button** on Agri-Wand
2. **Watch display**: "📸 Analyzing..."
3. **Wait 5-10 seconds** (cloud takes a bit longer than local)
4. **See results** with sensor recommendations!

---

## Troubleshooting

### "Build Failed"
**Check Railway logs for errors**:
- Missing dependencies? → Check `requirements.txt`
- Python version? → Railway uses Python 3.11+

**Fix**: Push updated code to GitHub, Railway auto-redeploys

### "Service Crashed"
**Check**:
- Environment variable `GEMINI_API_KEY` is set correctly
- No typos in API key

**Fix**: Update variable in Railway dashboard

### "ESP32 can't connect"
**Check**:
- URL is correct (include `/predict` at the end)
- URL starts with `https://` (not `http://`)
- ESP32 has internet connection

**Test**:
```cpp
// In Serial Monitor, you should see:
📤 Sending to server: https://YOUR-URL.up.railway.app/predict
```

### "Slow responses (>30 seconds)"
**Normal for cloud!**
- First request after idle: 10-15 seconds (cold start)
- Subsequent requests: 5-10 seconds
- Local server was faster (3-5 seconds) but required laptop

---

## Cost & Limits

### Free Tier
- **$5 credit/month**
- **~500 analyses/month** (at $0.01 each)
- **500 hours/month** (21 days always-on)

### After Free Tier
- **$5/month** for unlimited usage
- **Worth it** for always-available service

### Alternative: Render.com (100% Free)
If you want **completely free**:
- Use Render.com instead
- Service sleeps after 15 min
- First request takes 30-60 seconds
- Still works, just slower

---

## ✅ Success Checklist

- [ ] Railway account created
- [ ] Project deployed from GitHub
- [ ] `GEMINI_API_KEY` environment variable added
- [ ] Deployment successful (green checkmark)
- [ ] Health endpoint returns "healthy"
- [ ] URL copied
- [ ] ESP32 updated with cloud URL
- [ ] Button press works
- [ ] Results appear with sensor insights

---

## What You Get

```
Before (Local):
- Works only at home
- Laptop must be on
- Same WiFi network only

After (Cloud):
- Works ANYWHERE with internet
- No laptop needed
- Use in field, greenhouse, farm
- Share with friends/farmers
- Professional product! 🎉
```

---

**Ready to deploy? Follow these steps and your Agri-Wand will work anywhere! 🚀**
