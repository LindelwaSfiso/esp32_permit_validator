from rest_framework import serializers

from core.models import Scan, Driver


class CaptureImageSerializer(serializers.Serializer):
	capture = serializers.ImageField(required=True)


class ManualVerificationSerializer(serializers.Serializer):
	barcode = serializers.CharField(max_length=100, required=True)


class DriverSerializer(serializers.ModelSerializer):
	full_name = serializers.CharField()
	gender = serializers.CharField(source="get_gender_display")
	is_valid = serializers.BooleanField(source="is_permit_valid")
	valid_up_to = serializers.DateTimeField(format="%Y-%m-%d")

	class Meta:
		model = Driver
		fields = '__all__'


class ScanSerializer(serializers.ModelSerializer):
	driver = DriverSerializer(read_only=True, many=False)

	class Meta:
		model = Scan
		fields = '__all__'
