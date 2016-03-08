using UnityEngine;
using System.Collections;

[ExecuteInEditMode]
public class PanoramicTripodController : MonoBehaviour {

	[Range(0, 360.0f)]
	public float Azimuth = 0; //Y

	[Range(-90.0f, 90.0f)]
	public float Elevation = 0; //X

	private Transform servoY;
	private Transform servoX;

	void Awake ()
	{
		findServos();	
	}

	void Start ()
	{
		findServos();
	}

	void OnEnable()
	{
		findServos();
	}

	void Update ()
	{
		Apply(Azimuth, Elevation);
	}

	public void Apply(float a, float e)
	{
		Azimuth = a;
		Elevation = e;

		servoY.localEulerAngles = new Vector3(0,Azimuth,0);
		servoX.localEulerAngles = new Vector3(Elevation,0,0);
	}

	private void findServos()
	{
		if(servoY == null)
		{
			servoY = transform.FindChild("servoY");
		}

		if(servoX == null)
		{
			servoX = servoY.FindChild("servoX");
		}
	}
}
