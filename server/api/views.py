import cv2
import numpy as np
from pyzbar.pyzbar import decode

from django.http import JsonResponse
from rest_framework import status
from rest_framework.decorators import api_view
from rest_framework.response import Response

from api.serializers import CaptureImageSerializer, ScanSerializer
from core.models import Scan, Driver


@api_view(["POST"])
def verify_barcode_scan(request):
    try:
        serializer = CaptureImageSerializer(request.data)
        if serializer.is_valid():
            image_numpy = np.array(bytearray(serializer.capture), dtype=np.uint8)
            frame = cv2.imdecode(image_numpy, -1)

            for barcode in decode(frame):
                data = barcode.data.decode("utf-8")
                # save the scanned data
                driver = Driver.objects.filter(barcode=data)
                if driver:
                    driver = driver.first()

                    scan = Scan.objects.create(
                        driver=driver,
                        is_keypad=False,
                        saved_scan=serializer.capture
                    )
                    print(scan)

                    Response({
                        "message": "successful"
                    }, status=status.HTTP_200_OK)
                break

    except Exception:
        pass

    return Response({
        'error': 'Failed to decode file....'
    }, status=status.HTTP_500_INTERNAL_SERVER_ERROR)


def load_latest_scan(request):
    """
    AJAX endpoint, for loading or getting the latest police scan.
    Simulates realtime communication.
    """
    is_ajax = request.META.get('HTTP_X_REQUESTED_WITH') == 'XMLHttpRequest'
    if is_ajax:
        latest_scan = Scan.objects.latest()
        return JsonResponse({
            'message': "Query successful..",
            'scan': ScanSerializer(latest_scan).data
        }, status=200)

    return JsonResponse({
        'error': 'Method not allowed.'
    }, status=404)


@api_view(["POST"])
def manual_barcode_verification(request):
    pass
