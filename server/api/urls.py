from django.urls import path
from api.views import verify_barcode_scan, manual_barcode_verification, load_latest_scan

app_name = "api"
urlpatterns = [
    path('upload/', verify_barcode_scan),
    path('manual-verification/', manual_barcode_verification),
    path('load-latest-scan/', load_latest_scan),
]
