from io import BytesIO

from PIL import Image
from rest_framework import serializers

from core.models import Scan, Driver


class CaptureImageSerializer(serializers.Serializer):
	capture = serializers.ImageField(required=True)

	# def validate_capture(self, value):
	# 	if value.content_type == 'image/bmp':
	# 		try:
	# 			# Convert BMP to PNG
	# 			img = Image.open(value)
	# 			output = BytesIO()
	# 			img.save(output, format='PNG')
	# 			output.seek(0)
	# 			# Create a new InMemoryUploadedFile with the PNG data
	# 			from django.core.files.uploadedfile import InMemoryUploadedFile
	# 			new_image = InMemoryUploadedFile(
	# 				output,
	# 				'image',
	# 				"converted_image.png",
	# 				'image/png',
	# 				output.getbuffer().nbytes,
	# 				None
	# 			)
	# 			return new_image
	# 		except Exception as e:
	# 			raise serializers.ValidationError(f"BMP conversion failed: {e}")
	# 	return value


class ManualVerificationSerializer(serializers.Serializer):
	barcode = serializers.CharField(max_length=100, required=True)


class DriverSerializer(serializers.ModelSerializer):
	class Meta:
		model = Driver


class ScanSerializer(serializers.ModelSerializer):
	driver = DriverSerializer(read_only=True, many=False)

	class Meta:
		model = Scan
