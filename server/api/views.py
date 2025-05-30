from PIL import Image
from io import BytesIO

from django.http import JsonResponse
from rest_framework import status
from rest_framework.decorators import api_view
from rest_framework.response import Response

from api.serializers import ScanSerializer, ManualVerificationSerializer
from core.models import Scan, Driver
from google import genai

GEMINI_API_KEY = "AIzaSyD8Me5mk5GTlhhiGjveoDbvE_vM0FpXfT4"

# Only run this block for Gemini Developer API
client = genai.Client(api_key=GEMINI_API_KEY)


@api_view(["POST"])
def verify_barcode_scan(request):
	"""
	Note: Returns 200 status code for all, this is anti-api-patterns
	"""

	try:
		if request.content_type != 'image/jpeg':
			return Response({
				'error': f'Unsupported media type "{request.content_type}" in request. Expected image/jpeg.',
				'lcd1': "E400: Media Error"[:16],
				'lcd2': "Expected JPEG"[:16]
			}, status=status.HTTP_200_OK)

		image_bytes = request.body
		try:
			img = Image.open(BytesIO(image_bytes))
		except Exception as pil_error:
			print(f"Pillow Error: {pil_error}")
			return Response({
				'error': f'Failed to load image using Pillow: {pil_error}',
				'lcd1': 'E400, received',
				'lcd2': 'corrupted img!'
			}, status=status.HTTP_200_OK)

		try:
			import os
			img = Image.open(os.path.join('api/image.png'))
			response = client.models.generate_content(
				model="gemini-2.0-flash",
				contents=[
					img,
					"What is the barcode in this image? If there is no barcode, say 'No barcode found'. "
					"Only output the raw barcode value if found."
				]
			)

			if not response.text:
				print("barcodes not found")
				return Response({
					'details': 'No barcodes found in the image',
					'lcd1': 'E404, found NO',
					'lcd2': 'barcode in img'
				}, status=status.HTTP_200_OK)

			# checks the first barcode only
			data = str(response.text).strip().replace(" ", "")
			print("GEMINI RETURNED THIS: ", response.text)

			# save the scanned data
			driver = Driver.objects.filter(barcode=data).first()
			if driver:
				Scan.objects.create(
					driver=driver,
					is_keypad=False,
					saved_scan=img
				)

				if driver.is_permit_valid():
					return Response({
						"details": "Barcode verification successful. Permit valid.",
						"lcd1": f"Permit OK: {driver.format_date()}"[:16],
						"lcd2": f"Driver: {driver.get_initials()}",
					}, status=status.HTTP_200_OK)

				else:
					return Response({
						"details": "Barcode verification successful. Permit invalid.",
						"lcd1": f"Expired: {driver.format_date()}"[:16],
						"lcd2": f"Driver: {driver.get_initials()}",
					}, status=status.HTTP_200_OK)

			else:
				print(f'Driver with barcode {data} not found.')
				return Response({
					'details': f"Driver with barcode {data} not found.",
					'lcd1': "Driver Not Found",
					'lcd2': f"E404: {data[:10]}",
				}, status=status.HTTP_200_OK)

		except Exception as geminiApi:
			print(f"Decode GeminiAPI: {geminiApi}")
			return Response({
				'error': f'Failed to decode barcode: {geminiApi}',
				'lcd1': "E400, failed to.",
				'lcd2': f"decode img[bad]",
			}, status=status.HTTP_200_OK)

	except Exception as e:
		print("General Error: ", e)
		return Response({
			'details': f"Internal server error: {e}",
			'lcd1': "Server Error"[:16],
			'lcd2': "Try Again"[:16]
		}, status=status.HTTP_200_OK)


@api_view(["GET"])
def load_latest_scan(request):
	"""
	AJAX endpoint, for loading or getting the latest police scan.
	Simulates realtime communication.
	"""
	is_ajax = request.META.get('HTTP_X_REQUESTED_WITH') == 'XMLHttpRequest'
	if is_ajax or True:
		latest_scan = Scan.objects.last()
		return JsonResponse({
			'message': "Query successful..",
			'scan': ScanSerializer(latest_scan, context={'request': request}).data
		}, status=200)

	return JsonResponse({
		'error': 'Method not allowed.'
	}, status=404)


@api_view(["POST"])
def manual_barcode_verification(request):
	try:
		print("Request DATA: ", request.data)
		serializer = ManualVerificationSerializer(data=request.data)
		if serializer.is_valid():
			print("Serializer valid")
			driver = Driver.objects.filter(barcode=serializer.validated_data['barcode']).first()
			if driver:
				Scan.objects.create(
					driver=driver,
					is_keypad=True,
				)

				if driver.is_permit_valid():
					return Response({
						"details": "Barcode verification successful. Permit valid.",
						"lcd1": f"Permit OK: {driver.format_date()}"[:16],
						"lcd2": f"Driver: {driver.get_initials()}",
					}, status=status.HTTP_200_OK)

				else:
					return Response({
						"details": "Barcode verification successful. Permit invalid.",
						"lcd1": f"Expired: {driver.format_date()}"[:16],
						"lcd2": f"Driver: {driver.get_initials()}",
					}, status=status.HTTP_404_NOT_FOUND)

			else:
				return Response({
					'details': f"Driver with barcode {serializer.validated_data['barcode']} not found.",
					'lcd1': "Driver Not Found",
					'lcd2': f"E404: {serializer.validated_data['barcode'][:10]}",
				}, status=status.HTTP_404_NOT_FOUND)

		print("Not valid, ", serializer.errors)
		return Response({
			'error': "Invalid data format. Please check your input.",
			'lcd1': "E400: Data Error....."[:16],
			'lcd2': "Check Input....."[:16]
		}, status=status.HTTP_400_BAD_REQUEST)

	except Exception as e:
		print("Internal Error: ", e)
		return Response({
			'details': f"Internal server error: {e}",
			'lcd1': "Server Error"[:16],
			'lcd2': "Try Again"[:16]
		}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)
