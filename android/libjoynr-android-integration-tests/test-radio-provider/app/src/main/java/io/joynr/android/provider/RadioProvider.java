package io.joynr.android.provider;

import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.exceptions.ProviderRuntimeException;
import joynr.vehicle.Country;
import joynr.vehicle.GeoPosition;
import joynr.vehicle.Radio.AddFavoriteStationErrorEnum;
import joynr.vehicle.RadioAbstractProvider;
import joynr.vehicle.RadioStation;

public class RadioProvider extends RadioAbstractProvider {

    public static final String TAG = RadioProvider.class.getSimpleName();

    public static final long DELAY_MS = 2000;
    public static final String MISSING_NAME = "MISSING_NAME";

    private RadioStation currentStation;
    private List<RadioStation> stationsList = new ArrayList<RadioStation>();
    private Map<Country, GeoPosition> countryGeoPositionMap = new HashMap<>();

    private int currentStationIndex = 0;
    private ScheduledExecutorService executorService;

    public RadioProvider() {
        stationsList.add(new RadioStation("JoynrStation", true, Country.AUSTRALIA));
        stationsList.add(new RadioStation("Radio Popolare", false, Country.ITALY));
        stationsList.add(new RadioStation("JAZZ.FM91", false, Country.CANADA));
        stationsList.add(new RadioStation("Bayern 3", true, Country.GERMANY));
        countryGeoPositionMap.put(Country.AUSTRALIA, new GeoPosition(-37.8141070, 144.9632800)); // Melbourne
        countryGeoPositionMap.put(Country.ITALY, new GeoPosition(46.4982950, 11.3547580)); // Bolzano
        countryGeoPositionMap.put(Country.CANADA, new GeoPosition(53.5443890, -113.4909270)); // Edmonton
        countryGeoPositionMap.put(Country.GERMANY, new GeoPosition(48.1351250, 11.5819810)); // Munich
        currentStation = stationsList.get(currentStationIndex);
        executorService = Executors.newScheduledThreadPool(1);
    }

    @Override
    public Promise<Deferred<RadioStation>> getCurrentStation() {
        Deferred<RadioStation> deferred = new Deferred<>();
        Log.i(TAG, "getCurrentSation -> " + currentStation);
        deferred.resolve(currentStation);
        return new Promise<>(deferred);
    }

    @Override
    public Promise<DeferredVoid> shuffleStations() {
        final DeferredVoid deferred = new DeferredVoid();
        executorService.schedule(() -> {
            RadioStation oldStation = currentStation;
            currentStationIndex++;
            currentStationIndex = currentStationIndex % stationsList.size();
            currentStation = stationsList.get(currentStationIndex);
            currentStationChanged(currentStation);
            Log.i(TAG, "shuffleStations: " + oldStation + " -> " + currentStation);
            deferred.resolve();
        }, DELAY_MS, TimeUnit.MILLISECONDS);

        return new Promise<>(deferred);
    }

    @Override
    public Promise<AddFavoriteStationDeferred> addFavoriteStation(final RadioStation radioStation) {
        final AddFavoriteStationDeferred deferred = new AddFavoriteStationDeferred();

        if (radioStation.getName().isEmpty()) {
            deferred.reject(new ProviderRuntimeException(MISSING_NAME));
        }

        executorService.schedule(() -> {
            boolean duplicateFound = false;
            for (RadioStation station : stationsList) {
                if (!duplicateFound && station.getName().equals(radioStation.getName())) {
                    duplicateFound = true;
                    deferred.reject(AddFavoriteStationErrorEnum.DUPLICATE_RADIOSTATION);
                    break;
                }
            }
            if (!duplicateFound) {
                Log.i(TAG, "addFavoriteStation(" + radioStation + ")");
                stationsList.add(radioStation);
                deferred.resolve(true);
            }
        }, DELAY_MS, TimeUnit.MILLISECONDS);
        return new Promise<>(deferred);
    }

    @Override
    public Promise<GetLocationOfCurrentStationDeferred> getLocationOfCurrentStation() {
        Country country = currentStation.getCountry();
        GeoPosition location = countryGeoPositionMap.get(country);
        Log.i(TAG, "getLocationOfCurrentStation: country: " + country.name() + ", location: " + location);
        joynr.vehicle.RadioProvider.GetLocationOfCurrentStationDeferred deferred = new GetLocationOfCurrentStationDeferred();
        deferred.resolve(country, location);
        return new Promise<>(deferred);
    }
}
