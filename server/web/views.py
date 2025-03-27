from django.contrib.auth.decorators import login_required
from django.shortcuts import render

from core.models import Scan


@login_required(login_url="admin:login")
def index(request):
    latest_capture = Scan.objects.last()
    return render(request, "web/index.html", {
        'valid':  Scan.objects.last() is not None,
        'capture': latest_capture
    })
