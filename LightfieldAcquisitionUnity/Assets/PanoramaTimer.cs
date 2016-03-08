using UnityEngine;
using System.Collections;
using UnityEngine.Assertions;
using System.IO;
using System;

public class PanoramaTimer : MonoBehaviour {

	CameraArray mArray;

	string dir;

	void Awake()
	{
		dir = "./../dragonRedGreen/" + DateTime.UtcNow.ToString("yyyy-MM-dd-HH-mm-ss") + "/";

		if(!System.IO.Directory.Exists(dir))
		{
			System.IO.Directory.CreateDirectory(dir);
		}

		mArray = GameObject.FindObjectOfType<CameraArray>();
		Assert.IsNotNull(mArray);

		QualitySettings.vSyncCount = 0;  // VSync must be disabled
		Application.targetFrameRate = 5;
	}

	void Start ()
	{
		StartCoroutine(Snap());
	}
	

	void Update ()
	{
		
	}

	IEnumerator Snap()
	{
		for(int s = 0; s < mArray.xSamples; s++)
		{
			for(int t = 0; t < mArray.ySamples; t++)
			{
				mArray.setPosition(s, t);

				yield return new WaitForEndOfFrame();

				string filename = "frame_s_" + s + "_t_" + t + ".png";
				Application.CaptureScreenshot(dir + filename);

//				// Create a texture the size of the screen, RGB24 format
//				int width = Screen.width;
//				int height = Screen.height;
//				Texture2D tex = new Texture2D(width, height, TextureFormat.RGB24, false);
//				
//				// Read screen contents into the texture
//				tex.ReadPixels(new Rect(0, 0, width, height), 0, 0);
//				tex.Apply();
//				
//				// Encode texture into PNG
//				byte[] bytes = tex.EncodeToPNG();
//				Destroy(tex);
//				
//				// For testing purposes, also write to a file in the project folder
//
//				File.WriteAllBytes(dir + filename, bytes);
//				Debug.Log(filename);
			}
		}

		yield return null;

	}

//	private bool takeHiResShot = false;
//	public static string ScreenShotName(int width, int height) {
//		return string.Format("{0}/screenshots/screen_{1}x{2}_{3}.png", 
//		                     Application.dataPath, 
//		                     width, height, 
//		                     System.DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss"));
//	}
//	public void TakeHiResShot() {
//		takeHiResShot = true;
//	}
//	void LateUpdate() {
//		takeHiResShot |= Input.GetKeyDown("k");
//		if (takeHiResShot) {
//			RenderTexture rt = new RenderTexture(resWidth, resHeight, 24);
//			camera.targetTexture = rt;
//			Texture2D screenShot = new Texture2D(resWidth, resHeight, TextureFormat.RGB24, false);
//			camera.Render();
//			RenderTexture.active = rt;
//			screenShot.ReadPixels(new Rect(0, 0, resWidth, resHeight), 0, 0);
//			camera.targetTexture = null;
//			RenderTexture.active = null; // JC: added to avoid errors
//			Destroy(rt);
//			byte[] bytes = screenShot.EncodeToPNG();
//			string filename = ScreenShotName(resWidth, resHeight);
//			System.IO.File.WriteAllBytes(filename, bytes);
//			Debug.Log(string.Format("Took screenshot to: {0}", filename));
//			takeHiResShot = false;
//		}
//	}
}
