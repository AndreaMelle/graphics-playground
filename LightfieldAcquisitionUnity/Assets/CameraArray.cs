using UnityEngine;
using System.Collections;

[ExecuteInEditMode]
public class CameraArray : MonoBehaviour
{
	public int xSamples = 16;
	public int ySamples = 16;
	public float captureInterval = 0.05f;

	public float gizmoRadius = 0.01f;

	public Camera targetCam;

	[Range(0, 15)]
	public int xPos = 0;

	[Range(0, 15)]
	public int yPos = 0;

	private float dx;
	private float dy;
	private float arrayWidth;
	private float arrayHeight;

	void OnAwake()
	{
		
	}

	void OnEnable()
	{
		if(targetCam == null)
			targetCam = Camera.main;
		recomputeParams();
	}

	void Update()
	{
		if(targetCam == null)
			targetCam = Camera.main;
		recomputeParams();

		Vector3 pos = new Vector3((float)xPos * dx - arrayWidth / 2.0f, (float)yPos * dy - arrayHeight / 2.0f,0);
		targetCam.transform.position = this.transform.position + pos;
	}

	public void setPosition(int s, int t)
	{
		xPos = Mathf.Clamp(s, 0, xSamples - 1);
		yPos = Mathf.Clamp(t, 0, ySamples - 1);
	}

	private void recomputeParams()
	{
		arrayWidth = captureInterval * (xSamples - 1);
		arrayHeight = captureInterval * (ySamples - 1);
		dx = arrayWidth / (xSamples - 1);
		dy = arrayHeight / (ySamples - 1);
	}

	void OnDrawGizmos()
	{
		Vector3 center = this.transform.position;

		Gizmos.color = Color.red;

		Vector3 pos;

		for(int s = 0; s < xSamples; ++s)
		{
			for(int t = 0; t < ySamples; ++t)
			{
				pos = new Vector3((float)s * dx - arrayWidth / 2.0f, (float)t * dy - arrayHeight / 2.0f,0);

				Gizmos.DrawWireSphere(center + pos, gizmoRadius);
			}
		}

		Gizmos.color = Color.white;
		Matrix4x4 temp = Gizmos.matrix;
		pos = new Vector3((float)xPos * dx - arrayWidth / 2.0f, (float)yPos * dy - arrayHeight / 2.0f,0);
		Gizmos.matrix = Matrix4x4.TRS(pos + center, Quaternion.identity, Vector3.one);
		Gizmos.DrawFrustum(Vector3.zero, targetCam.fieldOfView, 0.25f, 0.01f, targetCam.aspect);
		Gizmos.matrix = temp;
	}

}
