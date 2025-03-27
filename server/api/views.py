import numpy as np
from PIL import Image
from io import BytesIO
from pyzbar.pyzbar import decode

from django.http import JsonResponse
from rest_framework import status
from rest_framework.decorators import api_view
from rest_framework.response import Response

from api.serializers import CaptureImageSerializer, ScanSerializer, ManualVerificationSerializer
from core.models import Scan, Driver


@api_view(["POST"])
def verify_barcode_scan(request):
	"""
	Note: Returns 200 status code for all, this is anti-api-patterns
	"""

	try:
		serializer = CaptureImageSerializer(data=request.data)
		if serializer.is_valid():
			print("\nSerializer is valid\n")
			print(serializer.validated_data)

			try:
				# Load BMP using Pillow
				image_bytes = serializer.validated_data['capture'].read()
				img = Image.open(BytesIO(image_bytes))
				frame = np.array(img)  # Convert Pillow image to NumPy array
			except Exception as pil_error:
				print(f"Pillow Error: {pil_error}")
				return Response({
					'error': f'Failed to load image using Pillow: {pil_error}',
					'lcd1': 'E400, received',
					'lcd2': 'corrupted img!'
				}, status=status.HTTP_400_BAD_REQUEST)

			try:
				barcodes = decode(frame)
				if not barcodes:
					print("barcodes not found")
					return Response({
						'details': 'No barcodes found in the image',
						'lcd1': 'E404, found NO',
						'lcd2': 'barcode in img'
					}, status=status.HTTP_404_NOT_FOUND)

				for barcode in barcodes:
					# checks the first barcode only
					data = barcode.data.decode("utf-8")
					data = str(data).strip().replace(" ", "")

					# save the scanned data
					driver = Driver.objects.filter(barcode=data).first()
					if driver:
						Scan.objects.create(
							driver=driver,
							is_keypad=False,
							saved_scan=serializer.validated_data['capture']
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

			except Exception as decode_error:
				print(f"Decode Error: {decode_error}")
				return Response({
					'error': f'Failed to decode barcode: {decode_error}',
					'lcd1': "E400, failed to.",
					'lcd2': f"decode img[bad]",
				}, status=status.HTTP_200_OK)

		print("Serialization Errors: ", serializer.errors)
		return Response({
			'error': "Invalid data format. Please check your input.",
			'lcd1': "E400: Data Error....."[:16],
			'lcd2': "Check Input....."[:16]
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
		serializer = ManualVerificationSerializer(data=request.data)
		if serializer.is_valid():
			driver = Driver.objects.filter(barcode=serializer.barcode).first()
			if driver:
				Scan.objects.create(
					driver=driver,
					is_keypad=False,
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
					'details': f"Driver with barcode {serializer.barcode} not found.",
					'lcd1': "Driver Not Found",
					'lcd2': f"E404: {serializer.barcode[:10]}",
				}, status=status.HTTP_404_NOT_FOUND)

		return Response({
			'error': "Invalid data format. Please check your input.",
			'lcd1': "E400: Data Error....."[:16],
			'lcd2': "Check Input....."[:16]
		}, status=status.HTTP_400_BAD_REQUEST)

	except Exception as e:
		return Response({
			'details': f"Internal server error: {e}",
			'lcd1': "Server Error"[:16],
			'lcd2': "Try Again"[:16]
		}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)
