from rest_framework import serializers

from core.models import Scan, Driver


class CaptureImageSerializer(serializers.Serializer):
    capture = serializers.ImageField()


class DriverSerializer(serializers.ModelSerializer):
    class Meta:
        model = Driver


class ScanSerializer(serializers.ModelSerializer):
    driver = DriverSerializer(read_only=True, many=False)

    class Meta:
        model = Scan
